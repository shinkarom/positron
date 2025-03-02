#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>

#include "sqlite3.h"
#include <quickjs.h>
#include "m4p.h"
#include "miniz.h"

std::vector<int16_t> soundBuffer(audioFramesPerTick*2);
std::vector<int32_t> frameBuffer(screenWidth * screenHeight);

sqlite3* db;

JSRuntime* runtime;
JSContext* context;

std::vector<char> trackBuffer;

int gameState;

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

void printJSException() {
    // Get the exception value
    JSValue exception = JS_GetException(context);
    
    // Try to retrieve the error message
    const char *error_str = JS_ToCString(context, exception);
    if (error_str) {
        // Print the exception message
        fprintf(stderr, "JS Exception: %s\n", error_str);
        JS_FreeCString(context, error_str);
    } else {
        // If we couldn't retrieve the error message, log a fallback message
        fprintf(stderr, "JS Exception occurred, but could not retrieve message.\n");
    }

    // Try to retrieve the stack trace, if it exists
    JSValue stack_val = JS_GetPropertyStr(context, exception, "stack");
    if (!JS_IsUndefined(stack_val)) {
        // If the stack property exists, retrieve and print it
        const char *stack_str = JS_ToCString(context, stack_val);
        if (stack_str) {
            std::cerr << "Stack Trace:\n" << stack_str << std::endl;
            JS_FreeCString(context, stack_str);
        } else {
            std::cerr << "Stack trace exists, but couldn't retrieve its string.\n";
        }
    }

    // Free the exception value to avoid memory leaks
    JS_FreeValue(context, exception);
}

void posiAPICls(uint32_t color) {
	color = 0xFF000000 | (color & 0x00FFFFFF);
	for(int i = 0; i < screenWidth * screenHeight; i++) {
		frameBuffer[i] = color;
	}
}

static JSValue js_api_cls(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    int32_t color;

    if (argc < 1) {
        fprintf(stderr, "Error: API_cls requires at least one argument (color).\n");
        return JS_EXCEPTION; // Indicate an error to JavaScript
    }

    if (!JS_IsNumber(argv[0])) {
        fprintf(stderr, "Error: API_cls argument 'color' must be a number.\n");
        return JS_EXCEPTION; // Indicate an error to JavaScript
    }

    if (JS_ToInt32(ctx, &color, argv[0])) {
        return JS_EXCEPTION; // Error converting argument to integer
    }

	posiAPICls(color);

    return JS_UNDEFINED; // Return undefined to JavaScript (no return value expected)
}

void posiPoweron() {	
	runtime = JS_NewRuntime();
	context = JS_NewContext(runtime);
	
	JSValue global_obj = JS_GetGlobalObject(context);

    /* Define API_cls function in global object */
    JS_SetPropertyStr(context, global_obj, "API_cls",
                      JS_NewCFunction(context, js_api_cls, "API_cls", 1)); // "API_cls" is function name in JS

    JS_FreeValue(context, global_obj);
	
	gameState = POSI_STATE_GAME;
}

void posiPoweroff() {
	JS_FreeContext(context);
	JS_FreeRuntime(runtime);
	
	sqlite3_close(db);
}

bool callTick() {
    JSValue global_obj, func, result, bgColorVal;
     // Host variable to store the JS value

    // Get the global object
    global_obj = JS_GetGlobalObject(context);

    // Get the function "API_Tick" from the global object
    func = JS_GetPropertyStr(context, global_obj, "API_Tick");

    if (!JS_IsFunction(context, func)) {
        // Function does not exist
        fprintf(stderr, "Error: API_Tick() is not defined in the script.\n");
        JS_FreeValue(context, func);
        JS_FreeValue(context, global_obj);
        return false;
    }

    // Call the function (no arguments, NULL means an empty argument list)
    result = JS_Call(context, func, global_obj, 0, NULL);

    // Check for exceptions
    if (JS_IsException(result)) {
		printJSException();
        JS_FreeValue(context, func);
        JS_FreeValue(context, global_obj);
        return false;
    }

    // Get the variable "API_BgColor" from the global object
    bgColorVal = JS_GetPropertyStr(context, global_obj, "API_BgColor");

    // Cleanup
    JS_FreeValue(context, bgColorVal);
    JS_FreeValue(context, result);
    JS_FreeValue(context, func);
    JS_FreeValue(context, global_obj);

    return true;
}

bool posiRun() {
	switch(gameState) {
		case POSI_STATE_GAME:
			return posiStateGameRun();
		default:
			return false;
	}
}

bool posiLoad(std::string fileName) {
	if(db) {
		sqlite3_close(db);
	}

	if (sqlite3_open(fileName.c_str(), &db) != SQLITE_OK) {
        std::cout << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
	
	std::string retrieved_code;
	sqlite3_stmt* stmt2;
	if(sqlite3_prepare_v2(db, "select data, compressed from code", -1, &stmt2, nullptr) != SQLITE_OK) {
		std::cout<<"error "<<sqlite3_errmsg(db)<<std::endl;
		return false;
	} 

	if(sqlite3_step(stmt2) != SQLITE_ROW) {
		std::cout<<"error "<<sqlite3_errmsg(db)<<std::endl;
	}
	
	const void* blobData = sqlite3_column_blob(stmt2, 0);
    int blobSize = sqlite3_column_bytes(stmt2, 0);
    bool isCompressed = sqlite3_column_int(stmt2, 1);
	
	std::vector<uint8_t> storedData(static_cast<const uint8_t*>(blobData), 
                                        static_cast<const uint8_t*>(blobData) + blobSize);

	if(isCompressed) {
		auto result = decompress(storedData);  // true = return as string
		retrieved_code = std::string(result.begin(), result.end());
	} else {
		retrieved_code = std::string(storedData.begin(), storedData.end());
	}
	
	
	sqlite3_finalize(stmt2);

    // Evaluate the JavaScript code
    auto r = JS_Eval(context, retrieved_code.c_str(), strlen(retrieved_code.c_str()), "<main>", JS_EVAL_TYPE_GLOBAL);
	if(JS_IsException(r)){
		printJSException();
		return false;
	}
	
	return true;
}

int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void posiRedraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer.data(),screenHeight*screenWidth*4);
}

void posiChangeState(int newState) {
	switch(gameState) {
		case POSI_STATE_GAME:
			break;
		default:
			break;
	}
	switch(newState) {
		case POSI_STATE_GAME:
			break;
		default:
			break;
	}
}

bool posiStateGameRun() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	return callTick();
}