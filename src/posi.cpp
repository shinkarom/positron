#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>

#include "sqlite3.h"
#include <quickjs.h>

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

void posi_run() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	frameBuffer[50*screenWidth+50] = 0xFFFFFFFF;
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
    const char *js_code = "3+2;";
	
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