#include "posi.h"

#include <vector>
#include <iostream>
#include <cstring>

std::vector<uint32_t> frameBuffer(maxScreenWidth * maxScreenHeight);
std::vector<uint32_t> tiles(numTiles * tileSide * tileSide);

void posiAPICls(uint32_t color) {
	color = 0xFF000000 | (color & 0x00FFFFFF);
	for(int i = 0; i < screenWidth * screenHeight; i++) {
		frameBuffer[i] = color;
	}
}

void posiAPIPutPixel(int x, int y, uint32_t color) {
	if(color == 0 || x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
		return;
	}
	auto newColor = 0xFF000000 | (color & 0x00FFFFFF);
	frameBuffer[y * screenWidth + x] = newColor;
}

void posiRedraw(uint32_t* buffer) {	
	memcpy(buffer, frameBuffer.data(),screenHeight*screenWidth*4);
}

void gpuLoad() {
	for(auto i = 0; i< 256; i++) {
		auto x = dbLoadByNumber("tiles", i);
		if(!x || (*x).size() != tilesPerPage * tileSide*tileSide*4) continue;
		auto outputStart = tiles.data() + i * (tilesPerPage * tileSide* tileSide);
		memcpy(outputStart, (*x).data(), tilesPerPage * tileSide* tileSide*4);
	}
}