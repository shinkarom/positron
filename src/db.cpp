#include "posi.h"

#include <iostream>
#include <format>
#include <utility>

#include "thirdparty/sqlite3.h"
#include "thirdparty/miniz.h"

sqlite3* db;

class SQLiteStatement {
	private:
		sqlite3_stmt* stmt = nullptr;
		sqlite3* m_db; // Keep a reference to the db for error messages

	public:
		// Prepare the statement in the constructor
		SQLiteStatement(sqlite3* db_handle, const std::string& query) : m_db(db_handle) {
			if (sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
				throw std::runtime_error("SQLite prepare error: " + std::string(sqlite3_errmsg(m_db)));
			}
		}

		// Finalize the statement in the destructor
		~SQLiteStatement() {
			if (stmt) {
				sqlite3_finalize(stmt);
			}
		}

		// Delete copy constructor and assignment to prevent double-freeing
		SQLiteStatement(const SQLiteStatement&) = delete;
		SQLiteStatement& operator=(const SQLiteStatement&) = delete;

		// Allow easy access to the underlying stmt pointer
		sqlite3_stmt* get() { return stmt; }
};

std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }

    mz_ulong maxCompressedSize = mz_compressBound(static_cast<mz_ulong>(data.size()));
     if (maxCompressedSize == 0 && !data.empty()) {
         throw std::runtime_error("Failed to calculate compressed bound for non-empty data.");
    }

    std::vector<uint8_t> compressedData(maxCompressedSize);
    mz_ulong compressedSize = maxCompressedSize;

    int result = mz_compress(compressedData.data(), &compressedSize, data.data(), static_cast<mz_ulong>(data.size()));

    if (result != MZ_OK) {
        std::string error_msg = "Compression failed with error code: " + std::to_string(result);
        throw std::runtime_error(error_msg);
    }

    compressedData.resize(compressedSize);

    return compressedData;
}

std::pair<std::vector<uint8_t>, int> conditional_compress(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> compressedData;
    try {
        compressedData = compress_data(data);
    } catch (const std::runtime_error& e) {
        throw e;
    }
    if (compressedData.size() < data.size()) {
        return std::make_pair(compressedData, 1);
    } else {
        return std::make_pair(data, 0);
    }
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> decompressedData(1024 * 1024);  
    mz_ulong decompressedSize = decompressedData.size();

    size_t result = mz_uncompress(decompressedData.data(), &decompressedSize, data.data(), data.size());
    
    if (result != MZ_OK) {
        throw std::runtime_error("Decompression failed!");
    }

    decompressedData.resize(decompressedSize);  // Resize to actual decompressed size
	
	return decompressedData;
}

static std::optional<std::vector<uint8_t>> extractAndDecompress(sqlite3_stmt* stmt) {
    const void* blobData = sqlite3_column_blob(stmt, 0);
    int blobSize = sqlite3_column_bytes(stmt, 0);
    bool isCompressed = sqlite3_column_int(stmt, 1);

    if (blobData == nullptr || blobSize == 0) {
        return std::vector<uint8_t>(); // Return empty vector if blob is null/empty
    }

    std::vector<uint8_t> storedData(
        static_cast<const uint8_t*>(blobData),
        static_cast<const uint8_t*>(blobData) + blobSize
    );

    if (isCompressed) {
        return decompress(storedData);
    } else {
        return storedData;
    }
}

std::optional<std::vector<uint8_t>> dbLoadByName(const std::string& type, const std::string& name) {
    try {
        std::string query = "SELECT data, compressed FROM data WHERE name=? AND type=?";
        SQLiteStatement stmt(db, query);

        sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, type.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            return extractAndDecompress(stmt.get());
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> dbLoadByNumber(const std::string& type, int number) {
    if (number < 0 || number > 255) {
        return std::nullopt;
    }
    auto name = std::format("{:02X}", number);
    return dbLoadByName(type, name);
}

bool dbIsSlotPresent(int slotNum) {
    if (slotNum < 0 || slotNum >= numSlots) {
        return false;
    }
    try {
        std::string query = "SELECT 1 FROM saves WHERE name=?";
        SQLiteStatement stmt(db, query);
        
        auto name = std::format("{:02X}", slotNum);
        sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);

        return (sqlite3_step(stmt.get()) == SQLITE_ROW);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        // An error is not the same as "not present", so re-throw or handle appropriately.
        // For simplicity here, we'll return false, but throwing would be better.
        return false; 
    }
}

// Deletes a specific slot. Returns true on success, false on failure.
bool dbSlotDelete(int slotNum) {
    if (slotNum < 0 || slotNum >= numSlots) {
        return false;
    }

    try {
        std::string query = "DELETE FROM saves WHERE name=?";
        SQLiteStatement stmt(db, query); // RAII ensures stmt is finalized

        auto name = std::format("{:02X}", slotNum);
        sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            std::cerr << "SQLite step error during delete: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        return true;

    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to delete slot " << slotNum << ": " << e.what() << std::endl;
        return false;
    }
}

// Deletes all slots. Returns true on success, false on failure.
bool dbSlotDeleteAll() {
    try {
        std::string query = "DELETE FROM saves";
        SQLiteStatement stmt(db, query); // RAII ensures stmt is finalized
        
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            std::cerr << "SQLite step error during delete all: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        return true;

    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to delete all slots: " << e.what() << std::endl;
        return false;
    }
}

bool dbSlotSave(int slotNum, const std::vector<uint8_t>& value) {
    if (slotNum < 0 || slotNum >= numSlots) {
        return false;
    }
    
    try {
        auto [dataToSave, isCompressedFlag] = conditional_compress(value);

        std::string query = "REPLACE INTO saves (name, data, compressed) VALUES (?, ?, ?)";
        SQLiteStatement stmt(db, query);
        
        auto name = std::format("{:02X}", slotNum);
        sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt.get(), 2, dataToSave.data(), static_cast<int>(dataToSave.size()), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt.get(), 3, isCompressedFlag);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            // Log the error but return false as requested.
            std::cerr << "SQLite step error during save: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        
        return true; // Success!

    } catch (const std::runtime_error& e) {
        // This catches errors from conditional_compress or SQLiteStatement construction.
        std::cerr << "Failed to save slot " << slotNum << ": " << e.what() << std::endl;
        return false;
    }
}

std::optional<std::vector<uint8_t>> dbSlotLoad(int slotNum) {
    if (slotNum < 0 || slotNum >= numSlots) {
        return std::nullopt;
    }

    try {
        // EFFICIENT: One query to find and load the data.
        std::string query = "SELECT data, compressed FROM saves WHERE name=?";
        SQLiteStatement stmt(db, query);

        auto name = std::format("{:02X}", slotNum);
        sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            return extractAndDecompress(stmt.get());
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to load slot " << slotNum << ": " << e.what() << std::endl;
    }

    // Return nullopt if not found OR if an error occurred.
    return std::nullopt;
}

bool dbVacuum() {
    if (!db) {
        std::cerr << "Cannot VACUUM: No active database connection." << std::endl;
        return false;
    }

    char* errMsg = nullptr;
    // sqlite3_exec is perfect for commands that don't return rows.
    // It is a wrapper around prepare, step, and finalize.
    if (sqlite3_exec(db, "VACUUM", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQLite VACUUM failed: " << errMsg << std::endl;
        sqlite3_free(errMsg); // The error message must be freed.
        return false;
    }

    return true;
}

void dbDisconnect() {
    if (!db) {
        return; // Already disconnected, nothing to do.
    }

    // Call the safe vacuum function before closing.
    dbVacuum();

    // Use sqlite3_close_v2 for better handling of busy resources.
    int rc = sqlite3_close_v2(db);
    if (rc == SQLITE_BUSY) {
        std::cerr << "Error: Could not close database because statements are still active." << std::endl;
        std::cerr << "This indicates a resource leak elsewhere in the code (a missing sqlite3_finalize)." << std::endl;
    } else if (rc != SQLITE_OK) {
        // This can happen if, for example, the close is interrupted.
        std::cerr << "Error closing database." << std::endl;
    }

    // THIS IS THE MOST IMPORTANT FIX:
    // Always null the pointer after attempting to close. This prevents other parts
    // of the code from using a dangling pointer or a handle that is "busy".
    db = nullptr;
}


bool dbTryConnect(const std::string& fileName) {
    // Always disconnect first to safely close any existing connection.
    dbDisconnect();

    // Use flags to open for reading/writing.
    // The SQLITE_OPEN_CREATE flag could be added if you want to create the DB if it doesn't exist.
    auto flags = SQLITE_OPEN_READWRITE;
    if (sqlite3_open_v2(fileName.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        // If sqlite3_open_v2 fails, it sets db to a valid pointer that contains error info.
        // We must close it to free its resources.
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db); // Clean up the error-state handle.
        db = nullptr;      // Ensure the global pointer is null on failure.
        return false;
    }

    return true;
}
/*
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
	auto r = sqlite3_close(db);
	if(r == SQLITE_BUSY) {
		std::cout<<"disconnect busy"<<std::endl;
	}
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
*/