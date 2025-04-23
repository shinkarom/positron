#include "posi.h"

#include <iostream>
#include <format>

#include "thirdparty/sqlite3.h"
#include "thirdparty/miniz.h"

sqlite3* db;

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

void dbDisconnect() {
	sqlite3_close(db);
}

bool dbTryConnect(std::string fileName){
	if(db) {
		dbDisconnect();
	}

	if (sqlite3_open(fileName.c_str(), &db) != SQLITE_OK) {
        std::cout << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    } else {
		return true;
	}
}