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
		if(!x || x->size() != tilesPerPage * tileSide*tileSide*2) continue;
		auto outputStart = tiles.data() + i * (tilesPerPage * tileSide* tileSide);
		uint8_t* dest = (uint8_t*)outputStart;
		const uint16_t* src = reinterpret_cast<const uint16_t*>((*x).data());

		for (size_t i = 0; i < tilesPerPage * tileSide * tileSide; i++) {
			uint16_t pixel = src[i];  // Read 16-bit ARGB1555 pixel

			// Extract alpha bit (1 if opaque, 0 if fully transparent)
			uint8_t a = (pixel & 0x8000) ? 0xFF : 0x00;

			// Extract and convert 5-bit RGB to 8-bit
			uint8_t r = ((pixel >> 10) & 0x1F) * 255 / 31;
			uint8_t g = ((pixel >> 5) & 0x1F) * 255 / 31;
			uint8_t b = (pixel & 0x1F) * 255 / 31;

			// If alpha is 0, force RGB to 0 as well
			if (a == 0) {
				r = 0;
				g = 0;
				b = 0;
			}

			// Store as ARGB8888
			*dest++ = b;  // Blue
			*dest++ = g;  // Green
			*dest++ = r;  // Red
			*dest++ = a;  // Alpha
		}
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