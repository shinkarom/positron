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
