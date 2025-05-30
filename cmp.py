import argparse
import sqlite3
import os
import zlib
from PIL import Image
import xml.etree.ElementTree as ET
import numpy as np
import base64
import struct
import time
from pathlib import Path 

def parse_arguments():
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(description="Create an SQLite database with a defined schema.")
    parser.add_argument("input_directory", help="The input directory containing 'code', 'tiles', and 'tilemap' subfolders.")
    parser.add_argument("database_name", help="The name of the SQLite database file to create.")
    return parser.parse_args()

def create_database_connection(database_name):
    """Creates and returns a connection to the SQLite database, replacing existing if present."""
    db_path = os.path.join(os.getcwd(), database_name)
   # if os.path.exists(db_path):
   #     try:
   #         os.remove(db_path)
   #         print(f"Existing SQLite database '{database_name}' has been replaced.")
   #     except OSError as e:
   #         print(f"Error removing existing database '{database_name}': {e}")
   #         return None
    try:
        conn = sqlite3.connect(db_path, isolation_level=None)
        print(f"SQLite database '{database_name}' created or opened successfully.")
        return conn
    except sqlite3.Error as e:
        print(f"An error occurred while connecting to the database: {e}")
        return None

def apply_schema(conn):
    """Applies the database schema."""
    if conn:
        try:
            cursor = conn.cursor()
            schema = """
            CREATE TABLE IF NOT EXISTS data (
                name TEXT NOT NULL,
                type TEXT NOT NULL,
                data BLOB,
                compressed INTEGER NOT NULL,
                CONSTRAINT un UNIQUE (name, type)
            );
            CREATE TABLE IF NOT EXISTS saves (
                name TEXT UNIQUE NOT NULL,
                data BLOB,
                compressed INTEGER NOT NULL
            );
            """
            cursor.executescript(schema)
            print("Database schema applied.")
        except sqlite3.Error as e:
            print(f"An error occurred while applying the schema: {e}")

def cond_compress_data(data):
    """
    Compresses the given bytearray or string using zlib.

    Args:
        data (bytearray or str): The data to compress.

    Returns:
        tuple: A tuple containing the compressed data (bytes) and an integer (1 if compressed is smaller, 0 otherwise).
    """
    if isinstance(data, str):
        data_bytes = data.encode('utf-8')
    elif isinstance(data, bytearray):
        data_bytes = bytes(data)
    elif isinstance(data, bytes):
        data_bytes = data
    else:
        raise TypeError(f"Input data must be a string or bytearray, got {type(data)}.")

    compressed_data = zlib.compress(data_bytes)
    #return compressed_data, 1
    if len(compressed_data) < len(data_bytes):
        return compressed_data, 1
    else:
        return data_bytes, 0

def insert_data(conn, data, name, type, is_compressed):
    """
    Compresses the given data and inserts it into the 'files' table.

    Args:
        conn (sqlite3.Connection): The database connection object.
        data (bytearray or str): The data to insert.
        name (str): The name of the data.
        type (str): The type of the data.
    """
    try:
        cursor = conn.cursor()
        cursor.execute("INSERT OR REPLACE INTO data (name, type, data, compressed) VALUES (?, ?, ?, ?)",
                       (name, type, sqlite3.Binary(data), is_compressed))
        conn.commit()
        print(f"'{type}' | '{name}' | {len(data)} | {is_compressed}")
    except sqlite3.Error as e:
        print(f"An error occurred while inserting data '{name}': {e}")

def _process_generic_files(conn, input_directory, subfolder, file_filter_logic, cache_extension, db_type, process_logic,db_mtime):
    target_directory = Path(input_directory, subfolder)
    if not target_directory.is_dir():
        print(f"Warning: '{subfolder}' subfolder not found in '{input_directory}'. Skipping {db_type} file processing.")
        return set()
    
    processed_entries = set()

    for filename in os.listdir(target_directory):
        # Use the passed file_filter_logic here
        if file_filter_logic(filename):
            filepath = Path(target_directory, filename)
            # Use filepath.stem for robustness, handles cases like .tar.gz
            name_without_extension = filepath.stem

            compressed_data = None
            is_compressed = None

            try:
                cursor = conn.cursor()
                cursor.execute("SELECT 1 FROM data WHERE name = ? AND type = ?", (name_without_extension, db_type))
                db_entry_exists = cursor.fetchone() is not None

                if (filepath.stat().st_mtime_ns > db_mtime) or (not db_entry_exists):                   
                    raw_content = process_logic(filepath)
                    compressed_data, is_compressed = cond_compress_data(raw_content)
                    insert_data(conn, compressed_data, name_without_extension, db_type, is_compressed)                    
            
                processed_entries.add((name_without_extension, db_type))
            
            except FileNotFoundError:
                print(f"Error: File not found: {filepath}")
            except Exception as e:
                print(f"Error processing {db_type} file '{filename}': {e}")
    return processed_entries

def filter_by_extension(extension):
    """Returns a filter function that checks if a filename ends with the given extension."""
    def _filter_func(filename):
        return filename.endswith(extension)
    return _filter_func

def _process_lua_content(filepath):
    """Reads Lua file content as UTF-8 bytes."""
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read().encode('utf-8')

def _process_tile_image_content(filepath):
    """Opens PNG image, extracts and reorders pixel data to BGRA bytearray."""
    img = Image.open(filepath).convert("RGBA")
    pixels = img.getdata()
    width, height = img.size
    
    tiles_data = bytearray()
    
    for i in range(128*128):
        tileNum = i // 64
        pxNum = i % 64
        tileRow = tileNum // 16
        tileCol = tileNum % 16
        tileY = pxNum // 8
        tileX = pxNum % 8
        pxX = tileCol*8+tileX
        pxY = tileRow*8+tileY
        r, g, b, a = img.getpixel((pxX,pxY))
        tiles_data.extend([b,g, r, a])
        
    
    #for pixel in pixels:
    #    r, g, b, a = pixel
    #    tiles_data.extend([b,g, r, a])

    return tiles_data


def _process_tilemap_content(filepath):
    """Parses TMX file, extracts and processes tile IDs, returns as bytearray."""
    tree = ET.parse(filepath)
    root = tree.getroot()
    output_data = bytearray()

    for layer in root.findall('layer'):
        data_element = layer.find('data')
        if data_element is not None and data_element.get('encoding') == 'base64' and data_element.get('compression') == 'zlib':
            encoded_data = data_element.text.strip()
            decoded_data = zlib.decompress(base64.b64decode(encoded_data))
            tile_ids = struct.unpack('<' + 'I' * (len(decoded_data) // 4), decoded_data)
            for tile_id in tile_ids:
                first_16_bits = tile_id & 0xFFFF
                if first_16_bits > 0:
                    first_16_bits -= 1
                bit_30 = (tile_id >> 30) & 1
                bit_31 = (tile_id >> 31) & 1
                combined_value = (bit_31<<15)|(bit_30<<14)|first_16_bits
                output_data.extend(combined_value.to_bytes(2, byteorder='little'))
        elif data_element is not None and data_element.get('encoding') == 'csv':
            tile_ids_str = data_element.text.strip().split(',')
            for tile_id_str in tile_ids_str:
                try:
                    tile_id = int(tile_id_str)
                    first_16_bits = tile_id & 0xFFFF
                    if first_16_bits > 0:
                        first_16_bits -= 1
                    bit_30 = (tile_id >> 30) & 1
                    bit_31 = (tile_id >> 31) & 1
                    combined_value = (bit_31<<15)|(bit_30<<14)|first_16_bits
                    output_data.extend(combined_value.to_bytes(2, byteorder='little'))
                except ValueError:
                    print(f"Warning: Could not parse tile ID '{tile_id_str}' in file '{filename}'.")
    return output_data

if __name__ == "__main__":
    args = parse_arguments()
    t = time.time()
    pat = Path(args.database_name)
    database_connection = create_database_connection(pat)
    if database_connection:
        mtime = pat.stat().st_mtime_ns
        apply_schema(database_connection)
        all_processed_entries = set()
        all_processed_entries.update(_process_generic_files(
            database_connection,
            args.input_directory,
            subfolder="code",
            file_filter_logic=filter_by_extension(".lua"), # Using the new filter function
            cache_extension="code",
            db_type="code",
            process_logic=_process_lua_content,
            db_mtime = mtime,
        ))
        all_processed_entries.update(_process_generic_files(
            database_connection,
            args.input_directory,
            subfolder="tiles",
            file_filter_logic=filter_by_extension(".png"), # Using the new filter function
            cache_extension="tiles",
            db_type="tiles",
            process_logic=_process_tile_image_content,
            db_mtime = mtime,
        ))
        all_processed_entries.update(_process_generic_files(
            database_connection,
            args.input_directory,
            subfolder="tilemap",
            file_filter_logic=filter_by_extension(".tmx"), # Using the new filter function
            cache_extension="tilemap",
            db_type="tilemap",
            process_logic=_process_tilemap_content,
            db_mtime = mtime,
        ))
        cursor = database_connection.cursor()
        cursor.execute("SELECT name, type FROM data;")
        all_db_entries = set(cursor.fetchall())
        obsolete_entries = all_db_entries - all_processed_entries
        if obsolete_entries:
            for name, type in obsolete_entries:
                try:
                    cursor.execute("DELETE FROM data WHERE name = ? AND type = ?", (name, type))
                    database_connection.commit()
                    print(f"Removed obsolete entry: Type='{type}', Name='{name}' from database.")
                except sqlite3.Error as e:
                    print(f"Error removing obsolete entry '{name}' ({type}): {e}")
            database_connection.commit() # Commit deletions
        
        database_connection.close()
    tt = time.time()
    print(f"Done in {tt-t} seconds")