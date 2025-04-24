#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>
#include <cmath>
#include <string>

int gameState;
std::string loadedFileName;

int screenWidth;
int screenHeight;

void posiPoweron() {	
	luaInit();
	gpuInit();
	apuInit();
	gameState = POSI_STATE_GAME;
	loadedFileName = "";
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
	dbDisconnect();
	if(!dbTryConnect(fileName)) {
		return false;
	}
	loadedFileName = fileName;
	gpuLoad();
	
	return luaLoad();
}

bool posiReset() {
	apuReset();
	gpuReset();
	return luaReset();
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

int16_t floatToInt16(float sample) {
	// Scale the float to the range of int16_t
  float scaledSample = sample * 32767.0f;

  // Clamp the value to the valid int16_t range to prevent overflow
  if (scaledSample > 32767.0f) {
    scaledSample = 32767.0f;
  } else if (scaledSample < -32768.0f) {
    scaledSample = -32768.0f;
  }

  // Round to the nearest integer and cast to int16_t
  return static_cast<int16_t>(std::round(scaledSample));
}

float int16ToFloat(int16_t sample) {
	// Scale the int16_t to the range of -1.0 to 1.0
	return static_cast<float>(sample) / 32767.0f;
}