#include "posi.h"

#include <vector>
#include <iostream>
#include <cstring>

std::vector<uint32_t> frameBuffer(screenWidth * screenHeight);
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
		if(!x || x->size() != tilesPerPage * tileSide*tileSide*4) continue;
		auto outputStart = tiles.data() + i * (tilesPerPage * tileSide* tileSide);
		memcpy(outputStart, x->data(), tilesPerPage * tileSide*tileSide*4);
	}
}

void posiAPIDrawSprite(int id, int w, int h, int x, int y, bool flipHorz, bool flipVert) {
	if(id < 0 || id > numTiles || w < 1 || h < 1 || x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
		return;
	}
	auto pageNum = id / tilesPerPage;
	auto idRemainder = id % tilesPerPage;
	auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
	auto tileRow = idRemainder / 16;
	auto tileColumn = idRemainder % 16;
	if(tileRow + w > 16 || tileColumn + h > 16){
		return;
	}
	constexpr auto pixelRowSize = 16 * tileSide;
	constexpr auto tileRowSize = pixelRowSize * tileSide;
	auto tileStart = pageStart + tileRow * tileRowSize + tileColumn * tileSide;
	for(auto yy = 0; yy < h*tileSide; yy++) {
		for(auto xx = 0; xx < w*tileSide; xx++){
			auto colorPos = tileStart + yy*pixelRowSize + xx;
			auto pixelColor = tiles[colorPos];
			auto posX = flipHorz ? x + w * tileSide - xx : x+xx;
			auto posY = flipVert ? y + h * tileSide - yy : y+yy;
			posiAPIPutPixel(posX,posY, pixelColor);
		}
	}
}