#include "posi.h"

#include <vector>

std::vector<int32_t> frameBuffer(screenWidth * screenHeight);

void posiAPICls(uint32_t color) {
	color = 0xFF000000 | (color & 0x00FFFFFF);
	for(int i = 0; i < screenWidth * screenHeight; i++) {
		frameBuffer[i] = color;
	}
}

void posiRedraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer.data(),screenHeight*screenWidth*4);
}