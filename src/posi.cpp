#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>
#include <array>

std::array<int16_t, audioFramesPerTick*2> soundBuffer;

int gameState;

void posiPoweron() {	
	luaInit();
	gpuInit();
	gameState = POSI_STATE_GAME;
}

void posiPoweroff() {
	luaDeinit();
	
	dbDisconnect();
}

bool posiRun() {
	switch(gameState) {
		case POSI_STATE_GAME:
			return posiStateGameRun();
		default:
			return false;
	}
}\



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

int16_t* posiAudiofeed() {
	return soundBuffer.data();
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
	return luaCallTick();
}