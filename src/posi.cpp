#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>

std::vector<int16_t> soundBuffer(audioFramesPerTick*2);

int gameState;



void posiPoweron() {	
	jsInit();
	
	gameState = POSI_STATE_GAME;
}

void posiPoweroff() {
	jsDeinit();
	
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
	
	auto retrieved_data = dbLoadByName("code", "MAIN");
	if(!retrieved_data) {
		std::cout<<"Error: Could not load MAIN script."<<std::endl;
		return false;
	}
	auto retrieved_code = std::string((*retrieved_data).begin(), (*retrieved_data).end());

	if(!jsEvalMain(retrieved_code)){
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
	return jsCallTick();
}