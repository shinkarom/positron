#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>

#include "sqlite3.h"
#include <quickjs.h>
#include "m4p.h"

int16_t soundBuffer[audioFramesPerTick*2];
int32_t frameBuffer[screenWidth * screenHeight];

sqlite3* db;

JSRuntime* runtime;
JSContext* context;

std::vector<char> trackBuffer;

int API_BgColor = 0xFF000000;

int gameState;

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


void posi_poweron() {	
	runtime = JS_NewRuntime();
	context = JS_NewContext(runtime);
	
	gameState = POSI_STATE_GAME;
}

void posi_poweroff() {
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

    // Check if the property exists and is a number
    if (JS_IsNumber(bgColorVal)) {
        JS_ToInt32(context, &API_BgColor, bgColorVal); // Convert to int
		API_BgColor = 0xFF000000 | (API_BgColor & 0x00FFFFFF);
       // printf("API_BgColor updated to: %d\n", API_BgColor);
    } else {
        //fprintf(stderr, "Error: API_BgColor is not a valid number.\n");
    }

    // Cleanup
    JS_FreeValue(context, bgColorVal);
    JS_FreeValue(context, result);
    JS_FreeValue(context, func);
    JS_FreeValue(context, global_obj);

    return true;
}

bool posi_run() {
	switch(gameState) {
		case POSI_STATE_GAME:
			return posi_state_game_run();
		default:
			return false;
	}
}

bool posi_load(std::string fileName) {
	if(db) {
		sqlite3_close(db);
	}
	auto res = sqlite3_open(fileName.c_str(), &db);
	if (res != SQLITE_OK) {
        std::cout << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
	
	std::string retrieved_code;
	sqlite3_stmt* stmt2;
	res = sqlite3_prepare_v2(db, "select scripttext from code", -1, &stmt2, nullptr);
	if(res) {
		std::cout<<"error "<<res<<std::endl;
	} else {
		res = sqlite3_step(stmt2);
		if(res != SQLITE_ROW) {
			std::cout<<"error "<<res<<std::endl;
		}
		retrieved_code = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0)));
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

int16_t* posi_audiofeed() {
	return &soundBuffer[0];
}

void posi_redraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer,screenHeight*screenWidth*4);
}

void posi_change_state(int newState) {
	
}

bool posi_state_game_run() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	for(int i = 0; i<screenWidth*screenHeight;i++){
		frameBuffer[i] = API_BgColor;
	}
	return callTick();
}