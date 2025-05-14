#include "posi.h"

#include <iostream>
#include <format>
#include <utility>

#include "thirdparty/sqlite3.h"
#include "thirdparty/miniz.h"

sqlite3* db;

std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data) {
    // Handle empty input data: compressing nothing results in nothing.
    if (data.empty()) {
        return {}; // Return an empty vector for empty input.
    }

    // Calculate the maximum possible size the compressed data could be.
    // mz_compressBound provides a safe upper bound based on the input size.
    // Casting to mz_ulong is necessary for the miniz function call.
    mz_ulong maxCompressedSize = mz_compressBound(static_cast<mz_ulong>(data.size()));

    // Basic check for edge cases, although mz_compressBound should be robust for non-empty inputs.
     if (maxCompressedSize == 0 && !data.empty()) {
         throw std::runtime_error("Failed to calculate compressed bound for non-empty data.");
    }

    // Prepare a buffer of the maximum possible size for the compressed data.
    std::vector<uint8_t> compressedData(maxCompressedSize);
    // This variable will hold the buffer's capacity initially and be updated by mz_compress
    // to the actual size of the compressed output data.
    mz_ulong compressedSize = maxCompressedSize;

    // Perform the compression.
    // mz_compress takes the destination buffer, its capacity (pointer), source data, and source size.
    int result = mz_compress(compressedData.data(), &compressedSize, data.data(), static_cast<mz_ulong>(data.size()));

    // Check the result of the compression operation. MZ_OK indicates success.
    if (result != MZ_OK) {
        // If the compression failed (e.g., due to memory issues, buffer error, etc.),
        // throw a runtime error. Including the miniz error code helps diagnose.
        std::string error_msg = "Compression failed with error code: " + std::to_string(result);
        // More specific error messages could be added here based on the value of 'result'.
        throw std::runtime_error(error_msg);
    }

    // Resize the compressed data vector down to the actual size returned by mz_compress.
    // This ensures the vector only contains the compressed data and no excess capacity.
    compressedData.resize(compressedSize);

    // Return the vector containing the compressed data.
    return compressedData;
}

std::pair<std::vector<uint8_t>, int> conditional_compress(const std::vector<uint8_t>& data) {
    // Call the core compression function to get the compressed data.
    std::vector<uint8_t> compressedData;
    try {
        compressedData = compress_data(data);
    } catch (const std::runtime_error& e) {
        // If compress_data throws an error, it means compression failed.
        // Re-throw the exception to indicate that the process could not complete.
        throw e;
    }

    // Compare the size of the newly compressed data to the size of the original input data.
    if (compressedData.size() < data.size()) {
        // If the compressed data is strictly smaller than the original data,
        // return the compressed data along with the indicator 1.
        return std::make_pair(compressedData, 1);
    } else {
        // If the compressed data is not smaller (i.e., it's equal or larger),
        // return the original data vector along with the indicator 0.
        return std::make_pair(data, 0);
    }
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) {
    // Prepare a buffer for decompression (initially 1MB)
    std::vector<uint8_t> decompressedData(1024 * 1024);  
    mz_ulong decompressedSize = decompressedData.size();

    // Decompress using miniz
    size_t result = mz_uncompress(decompressedData.data(), &decompressedSize, data.data(), data.size());
    
    if (result != MZ_OK) {
        throw std::runtime_error("Decompression failed!");
    }

    decompressedData.resize(decompressedSize);  // Resize to actual decompressed size
	
	return decompressedData;
}

std::optional<std::vector<uint8_t>> dbLoadByName(std::string tableName, std::string name) {
	sqlite3_stmt* stmt;
	std::string query = "select data, compressed from data where name=? and type=?";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return std::nullopt;
	} 
	sqlite3_bind_text(stmt, 1, name.c_str(),-1, nullptr);
	sqlite3_bind_text(stmt, 2, tableName.c_str(),-1, nullptr);

	if(sqlite3_step(stmt) != SQLITE_ROW) {
		return std::nullopt;
	}
	
	const void* blobData = sqlite3_column_blob(stmt, 0);
    int blobSize = sqlite3_column_bytes(stmt, 0);
    bool isCompressed = sqlite3_column_int(stmt, 1);
	
	std::vector<uint8_t> storedData(static_cast<const uint8_t*>(blobData), 
                                        static_cast<const uint8_t*>(blobData) + blobSize);
	
	sqlite3_finalize(stmt);
	
	if(isCompressed) {
		return decompress(storedData) ;
	} else {
		return storedData;
	}
}

std::optional<std::vector<uint8_t>> dbLoadByNumber(std::string tableName, int number) {
	if(number < 0 || number > 255) {
		return std::nullopt;
	}
	auto r = std::format("{:02X}", number);
	return dbLoadByName(tableName, r);
}

bool posiAPIIsSlotPresent(int slotNum) {
	if(slotNum<0 ||slotNum>=numSlots) {
		return false;
	}
	sqlite3_stmt* stmt;
	std::string query = "select 1 from saves where number=?";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return false;
	} 
	sqlite3_bind_int(stmt, 1, slotNum);

	if(sqlite3_step(stmt) != SQLITE_ROW) {
		return false;
	}
	
	auto result = sqlite3_column_int(stmt, 0);
	return result != 0;
}

bool posiAPISlotDelete(int slotNum) {
	if(slotNum<0 ||slotNum>=numSlots) {
		return false;
	}
	sqlite3_stmt* stmt;
	std::string query = "delete from saves where number=?";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return false;
	} 
	sqlite3_bind_int(stmt, 1, slotNum);

	if(sqlite3_step(stmt) != SQLITE_DONE) {
		return false;
	}

	return true;
}

bool posiAPISlotDeleteAll() {
	sqlite3_stmt* stmt;
	std::string query = "delete from saves";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return false;
	} 

	if(sqlite3_step(stmt) != SQLITE_DONE) {
		return false;
	}
	return true;
}

bool dbSlotSave(int number, std::vector<uint8_t>& value) {
	if(number < 0 || number >= numSlots) return false;
    // 1. Conditionally compress the input data.
    // This function returns the data to save (either original or compressed)
    // and an integer flag indicating if it was compressed.
    std::vector<uint8_t> dataToSave;
    int isCompressedFlag;
    try {
        std::pair<std::vector<uint8_t>, int> compressionResult = conditional_compress(value);
        dataToSave = compressionResult.first;
        isCompressedFlag = compressionResult.second;
    } catch (const std::runtime_error& e) {
        // If conditional_compress throws an exception, it means compression failed.
        // We log the error and return false to indicate the save failed.
        std::cerr << "Error during data compression for save slot " << number << ": " << e.what() << std::endl;
        return false; // Saving failed due to compression error.
    }

    // 2. Prepare the SQL statement for saving the data.
    sqlite3_stmt* stmt = nullptr; // Initialize statement pointer to nullptr
    // Using REPLACE INTO simplifies the logic; it acts as an INSERT if the 'number'
    // does not exist, or an UPDATE if it does.
    std::string query = "REPLACE INTO saves (number, data, compressed) VALUES (?, ?, ?)";

    // Prepare the SQL statement.
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        // If preparing the statement fails, print the SQLite error and return false.
        std::cerr << "SQLite prepare error for dbSlotSave (" << number << "): " << sqlite3_errmsg(db) << std::endl;
        // No need to finalize if prepare failed
        return false;
    }

    // 3. Bind the values to the prepared statement's parameters.
    // Bind the save slot number (INTEGER) to the first parameter (index 1).
    if (sqlite3_bind_int(stmt, 1, number) != SQLITE_OK) {
        std::cerr << "SQLite bind_int error for dbSlotSave (" << number << ", number): " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt); // Finalize statement before returning on error
        return false;
    }

    // Bind the data blob (BLOB) to the second parameter (index 2).
    // We use SQLITE_TRANSIENT which tells SQLite to make its own copy of the data.
    // This is important because 'dataToSave' is a local variable that might go
    // out of scope before SQLite finishes using the data if SQLITE_STATIC was used.
    // The size is cast to int, assuming the data size fits within an integer.
    if (sqlite3_bind_blob(stmt, 2, dataToSave.data(), static_cast<int>(dataToSave.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr << "SQLite bind_blob error for dbSlotSave (" << number << ", data): " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt); // Finalize statement before returning on error
        return false;
    }

    // Bind the compressed flag (INTEGER 0 or 1) to the third parameter (index 3).
     if (sqlite3_bind_int(stmt, 3, isCompressedFlag) != SQLITE_OK) {
        std::cerr << "SQLite bind_int error for dbSlotSave (" << number << ", compressed): " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt); // Finalize statement before returning on error
        return false;
    }

    // 4. Execute the prepared statement.
    // For INSERT or REPLACE statements, sqlite3_step returns SQLITE_DONE upon successful execution.
    int step_result = sqlite3_step(stmt);

    // 5. Check the execution result.
    if (step_result != SQLITE_DONE) {
        // If the result is not SQLITE_DONE, an error occurred during execution.
        std::cerr << "SQLite step error for dbSlotSave (" << number << "): " << sqlite3_errmsg(db) << " (Result code: " << step_result << ")" << std::endl;
        sqlite3_finalize(stmt); // Finalize statement before returning on error
        return false;
    }

    // 6. Finalize the statement to release resources.
    sqlite3_finalize(stmt);
    stmt = nullptr; // Set to nullptr after finalization

    // 7. Return true to indicate the save operation was successful.
    return true;
}

std::optional<std::vector<uint8_t>> dbSlotLoad(int number) {
	if(number < 0 || number >= numSlots || !posiAPIIsSlotPresent(number)) return std::nullopt;
	
	sqlite3_stmt* stmt;
	std::string query = "select data, compressed from saves where number=? ";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return std::nullopt;
	} 
	sqlite3_bind_int(stmt, 1, number);

	if(sqlite3_step(stmt) != SQLITE_ROW) {
		return std::nullopt;
	}
	
	const void* blobData = sqlite3_column_blob(stmt, 0);
    int blobSize = sqlite3_column_bytes(stmt, 0);
    bool isCompressed = sqlite3_column_int(stmt, 1);
	
	std::vector<uint8_t> storedData(static_cast<const uint8_t*>(blobData), 
                                        static_cast<const uint8_t*>(blobData) + blobSize);
	
	sqlite3_finalize(stmt);
	
	if(isCompressed) {
		return decompress(storedData) ;
	} else {
		return storedData;
	}	
}

bool dbVacuum() {
	sqlite3_stmt* stmt;
	std::string query = "vacuum";
	if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		std::cout<<"SQLite error: "<<sqlite3_errmsg(db)<<std::endl;
		return false;
	} 

	if(sqlite3_step(stmt) != SQLITE_DONE) {
		return false;
	}
	return true;
}

void dbDisconnect() {
	if(db) {dbVacuum();}
	sqlite3_close(db);
}

bool dbTryConnect(std::string fileName){
	if(db) {
		dbDisconnect();
	}
	auto flags = SQLITE_OPEN_READWRITE;
	if (sqlite3_open_v2(fileName.c_str(), &db,flags, nullptr) != SQLITE_OK) {
        std::cout << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    } else {
		return true;
	}
}