#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>

#include "sqlite3.h"
#include <quickjs.h>
#include <SQLiteCpp/SQLiteCpp.h>

int16_t soundBuffer[audioFramesPerTick*2];
int32_t frameBuffer[screenWidth * screenHeight];

sqlite3* db;

JSRuntime* runtime;
JSContext* context;

void posi_poweron() {
	sqlite3_open(":memory:", &db);
	char* errMsg;
	sqlite3_exec(db, "create table code (scripttext text)",nullptr, nullptr, &errMsg);
	if(errMsg!=nullptr) {
		std::cout<<errMsg<<std::endl;
		sqlite3_free(errMsg);
	}
	
	runtime = JS_NewRuntime();
	context = JS_NewContext(runtime);
}

void posi_poweroff() {
	JS_FreeContext(context);
	JS_FreeRuntime(runtime);
	
	sqlite3_close(db);
}

bool callTick() {
	JSValue global_obj, func, result;
    
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
        JSValue exception = JS_GetException(context);
        const char *error_str = JS_ToCString(context, exception);
        fprintf(stderr, "JS Exception: %s\n", error_str);
        JS_FreeCString(context, error_str);
        JS_FreeValue(context, exception);
		JS_FreeValue(context, result);
		JS_FreeValue(context, func);
		JS_FreeValue(context, global_obj);
		return false;
    } else {
        
    }

    // Cleanup
    JS_FreeValue(context, result);
    JS_FreeValue(context, func);
    JS_FreeValue(context, global_obj);
	return true;
}

bool posi_run() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	for(int i = 0; i<screenWidth*screenHeight;i++){
		frameBuffer[i] = 0xFF000000;
	}
	frameBuffer[50*screenWidth+50] = 0xFFFFFFFF;
	return callTick();
}

void posi_load() {
	std::string retrieved_code;
	sqlite3_stmt* stmt2;
	auto res = sqlite3_prepare_v2(db, "select scripttext from code", -1, &stmt2, nullptr);
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
    JS_Eval(context, retrieved_code.c_str(), strlen(retrieved_code.c_str()), "<input>", JS_EVAL_TYPE_GLOBAL);
	
	
}

void posi_save() {
	// A small JavaScript program to evaluate
    const char *js_code = "function API_Tick() {}";
	
	sqlite3_stmt* stmt1;
	auto res = sqlite3_prepare_v2(db, "insert into code values (?)", -1, &stmt1, nullptr);
	if(res) {
		std::cout<<"error "<<res<<std::endl;
	} else {
		sqlite3_bind_text(stmt1, 1, js_code, strlen(js_code), SQLITE_STATIC);
		res = sqlite3_step(stmt1);
		if(res != SQLITE_DONE) {
			std::cout<<"error "<<res<<std::endl;
		}
	}
	sqlite3_finalize(stmt1);
}

int16_t* posi_audiofeed() {
	return &soundBuffer[0];
}

void posi_redraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer,screenHeight*screenWidth*4);
}