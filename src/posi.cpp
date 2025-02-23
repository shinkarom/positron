#include <posi.h>

#include <cstdint>
#include <iostream>
#include <cstring>

#include "sqlite3.h"
#include <SDL3/SDL.h>
#include <quickjs.h>

int16_t soundBuffer[audioFramesPerTick*2];
int32_t frameBuffer[screenWidth * screenHeight];

void posi_poweron() {
	
}

void posi_poweroff() {

}

void posi_run() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	frameBuffer[50*screenWidth+50] = 0xFFFFFFFF;
}

void posi_load() {
	
}

void posi_save() {
	
}

int16_t* posi_audiofeed() {
	return &soundBuffer[0];
}

void posi_redraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer,screenHeight*screenWidth*4);
}