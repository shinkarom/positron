#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>

int gameState;

void posiPoweron() {	
	luaInit();
	gpuInit();
	apuInit();
	gameState = POSI_STATE_GAME;
}

void posiPoweroff() {
	luaDeinit();
	
	dbDisconnect();
}

bool posiRun() {
	apuProcess();
	switch(gameState) {
		case POSI_STATE_GAME: {}
			return posiStateGameRun();
		default:
			return false;
	}
}



bool posiLoad(std::string fileName) {
	if(!dbTryConnect(fileName)) {
		return false;
	}
	
	gpuLoad();
	
	auto retrieved_data = dbLoadByName("code", "main");
	if(!retrieved_data) {
		std::cout<<"Error: Could not load main script."<<std::endl;
		return false;
	}
	auto retrieved_code = std::string((*retrieved_data).begin(), (*retrieved_data).end());

	if(!luaEvalMain(retrieved_code)){
		return false;
	}
	
	return true;
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
	
	return luaCallTick();
}