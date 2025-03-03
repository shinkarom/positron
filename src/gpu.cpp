#include "posi.h"

#include <vector>

std::vector<uint32_t> frameBuffer(maxScreenWidth * maxScreenHeight);
std::vector<uint8_t> depthBuffer(maxScreenWidth * maxScreenHeight);

void posiAPICls(uint32_t color) {
	color = 0xFF000000 | (color & 0x00FFFFFF);
	for(int i = 0; i < screenWidth * screenHeight; i++) {
		frameBuffer[i] = color;
		depthBuffer[i] = 1;
	}
}

void posiAPIPutPixel(uint8_t depth, int x, int y, uint32_t color) {
	if(x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
		return;
	}
	auto oldDepth = depthBuffer[y * screenWidth + x];
	if(oldDepth > depth) {
		return;
	}
	auto newColor = 0xFF000000 | (color & 0x00FFFFFF);
	frameBuffer[y * screenWidth + x] = newColor;
	depthBuffer[y * screenWidth + x] = depth;
}

void posiRedraw(uint32_t* buffer) {
	memcpy(buffer, frameBuffer.data(),screenHeight*screenWidth*4);
}