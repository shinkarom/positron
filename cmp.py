import argparse
import sqlite3
import os
import zlib
from PIL import Image
import xml.etree.ElementTree as ET
from scipy.spatial import KDTree
import numpy as np
import base64
import struct

def parse_arguments():
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(description="Create an SQLite database with a defined schema.")
    parser.add_argument("input_directory", help="The input directory containing 'code', 'tiles', and 'tilemap' subfolders.")
    parser.add_argument("database_name", help="The name of the SQLite database file to create.")
    return parser.parse_args()

def create_database_connection(database_name):
    """Creates and returns a connection to the SQLite database, replacing existing if present."""
    db_path = os.path.join(os.getcwd(), database_name)
    if os.path.exists(db_path):
        try:
            os.remove(db_path)
            print(f"Existing SQLite database '{database_name}' has been replaced.")
        except OSError as e:
            print(f"Error removing existing database '{database_name}': {e}")
            return None
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
            """
            cursor.executescript(schema)
            print("Database schema applied (including 'files' table).")
        except sqlite3.Error as e:
            print(f"An error occurred while applying the schema: {e}")

def compress_data(data):
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
    else:
        raise TypeError("Input data must be a string or bytearray.")

    compressed_data = zlib.compress(data_bytes)

    if len(compressed_data) < len(data_bytes):
        return compressed_data, 1
    else:
        return data_bytes, 0

def insert_data_with_compression(conn, data, name, type):
    """
    Compresses the given data and inserts it into the 'files' table.

    Args:
        conn (sqlite3.Connection): The database connection object.
        data (bytearray or str): The data to insert.
        name (str): The name of the data.
        type (str): The type of the data.
    """
    compressed_data, is_compressed = compress_data(data)
    try:
        cursor = conn.cursor()
        cursor.execute("INSERT INTO data (name, type, data, compressed) VALUES (?, ?, ?, ?)",
                       (name, type, sqlite3.Binary(compressed_data), is_compressed))
        conn.commit()
        r = f"| {len(data)}" if is_compressed else""
        print(f"'{type}' | '{name}' | {len(compressed_data)} {r}")
    except sqlite3.Error as e:
        print(f"An error occurred while inserting data '{name}': {e}")

def process_lua_files(conn, input_directory):
    """
    Goes through all lua files in the 'code' subfolder of the input directory,
    conditionally compresses them, and inserts them into the database.

    Args:
        conn (sqlite3.Connection): The database connection object.
        input_directory (str): The path to the input directory.
    """
    code_directory = os.path.join(input_directory, "code")
    if not os.path.isdir(code_directory):
        print(f"Warning: 'code' subfolder not found in '{input_directory}'. Skipping Lua file processing.")
        return

    for filename in os.listdir(code_directory):
        if filename.endswith(".lua"):
            filepath = os.path.join(code_directory, filename)
            try:
                with open(filepath, 'r') as f:
                    content = f.read()
                name_without_extension = os.path.splitext(filename)[0]
                insert_data_with_compression(conn, content, name_without_extension, "code")
            except FileNotFoundError:
                print(f"Error: Lua file not found: {filepath}")
            except Exception as e:
                print(f"Error processing Lua file '{filename}': {e}")

def process_tile_images(conn, input_directory):
    """
    Goes through all png files in the 'tiles' subfolder of the input directory,
    creates two bytearrays, and inserts them into the database.

    Args:
        conn (sqlite3.Connection): The database connection object.
        input_directory (str): The path to the input directory.
    """
    tiles_directory = os.path.join(input_directory, "tiles")
    if not os.path.isdir(tiles_directory):
        print(f"Warning: 'tiles' subfolder not found in '{input_directory}'. Skipping PNG file processing.")
        return

    for filename in os.listdir(tiles_directory):
        if filename.endswith(".png"):
            filepath = os.path.join(tiles_directory, filename)
            try:
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

                name_without_extension = os.path.splitext(filename)[0]
                insert_data_with_compression(conn, tiles_data, name_without_extension, "tiles")

            except FileNotFoundError:
                print(f"Error: PNG file not found: {filepath}")
            except Exception as e:
                print(f"Error processing PNG file '{filename}': {e}")

def process_tilemap_files(conn, input_directory):
    """
    Goes through all tmx files in the 'tilemap' subfolder of the input directory,
    extracts tile data, processes it, and inserts into the database.

    Args:
        conn (sqlite3.Connection): The database connection object.
        input_directory (str): The path to the input directory.
    """
    tilemap_directory = os.path.join(input_directory, "tilemap")
    if not os.path.isdir(tilemap_directory):
        print(f"Warning: 'tilemap' subfolder not found in '{input_directory}'. Skipping TMX file processing.")
        return

    for filename in os.listdir(tilemap_directory):
        if filename.endswith(".tmx"):
            filepath = os.path.join(tilemap_directory, filename)
            try:
                tree = ET.parse(filepath)
                root = tree.getroot()
                name_without_extension = os.path.splitext(filename)[0]
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
                insert_data_with_compression(conn, output_data, name_without_extension, "tilemap")

            except FileNotFoundError:
                print(f"Error: TMX file not found: {filepath}")
            except ET.ParseError as e:
                print(f"Error parsing TMX file '{filename}': {e}")
            except Exception as e:
                print(f"Error processing TMX file '{filename}': {e}")

if __name__ == "__main__":
    args = parse_arguments()

    database_connection = create_database_connection(args.database_name)
    if database_connection:
        apply_schema(database_connection)
        process_lua_files(database_connection, args.input_directory)
        process_tile_images(database_connection, args.input_directory)
        process_tilemap_files(database_connection, args.input_directory)
        database_connection.close()