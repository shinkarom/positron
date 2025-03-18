#include "posi.h"

#include <iostream>
#include <cstring>
#include <array>

std::array<uint32_t, screenWidth * screenHeight> frameBuffer;
std::array<uint16_t, numTilesPixels> tiles;
std::array<uint16_t, tilemapTotalTiles> tilemaps[numTilemaps];
std::array<uint32_t, (1<<16)> palette;

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

void gpuInit() {
    for (uint32_t i = 0; i < (1 << 16); ++i) {
        if (i == 0) {
            // Color 0 is transparent (RGBA: 0x00000000)
            palette[0] = 0x00000000;
        } else {
            // Extract the bit fields from the 16-bit color
            uint16_t color16 = static_cast<uint16_t>(i);
            uint8_t a = (color16 >> 15) & 0x01;
            uint8_t r = (color16 >> 10) & 0x1F;
            uint8_t g = (color16 >> 5) & 0x1F;
            uint8_t b = color16 & 0x1F;

            // Construct the 8-bit Red, Green, and Blue components
            uint8_t red = (r << 3) | (a << 2);
            uint8_t green = (g << 3) | (a << 2);
            uint8_t blue = (b << 3) | (a << 2);

            // The alpha channel for non-transparent colors is 0xFF
            uint32_t alpha = 0xFF << 24;
            uint32_t red32 = red << 16;
            uint32_t green32 = green << 8;
            uint32_t blue32 = blue;

            // Combine the RGBA components into a 32-bit value
            palette[i] = alpha | red32 | green32 | blue32;
        }
    }
}

void loadTilePages() {
	for(auto i = 0; i< numTilePages; i++) {
		auto x = dbLoadByNumber("tiles", i);
		if(!x || x->size() != pixelsPerPage*2) continue;
		auto outputStart = tiles.data() + i * (pixelsPerPage);
		uint16_t* dest = (uint16_t*)outputStart;
		const uint16_t* src = reinterpret_cast<const uint16_t*>((*x).data());
		for (size_t i = 0; i < pixelsPerPage; i++) {
			uint16_t pixel = src[i];  // Read 16-bit ARGB1555 pixel
			*dest++ = pixel;
		}	
	}	
}

void loadTilemaps() {
	for(auto i = 0; i<numTilemaps; i++) {
		auto x = dbLoadByNumber("tilemaps", i);
		if(!x || x->size() != tilemapTotalBytes) continue;
		memcpy(tilemaps[i].data(), x->data(), tilemapTotalBytes);
	}
}

void gpuLoad() {
	loadTilePages();
	loadTilemaps();
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
	auto tileStart = pageStart + tileRow * tileRowSize + tileColumn * tileSide;
	for(auto yy = 0; yy < h*tileSide; yy++) {
		for(auto xx = 0; xx < w*tileSide; xx++){
			auto colorPos = tileStart + yy*pixelRowSize + xx;
			auto pixelColor = palette[tiles[colorPos]];
			auto posX = flipHorz ? x + w * tileSide - xx : x+xx;
			auto posY = flipVert ? y + h * tileSide - yy : y+yy;
			posiAPIPutPixel(posX,posY, pixelColor);
		}
	}
}

uint16_t posiAPIGetTilemapEntry(int tilemapNum, int tmx, int tmy) {
	if(tilemapNum < 0 ||tilemapNum >= numTilemaps||tmx<0||tmx>=tilemapTotalWidthTiles||tmy <0 || tmy >= tilemapTotalHeightTiles)
		return 0;
	auto r = tilemaps[tilemapNum][tmy*tilemapTotalWidthTiles+tmx];
	return r;
}

void posiAPISetTilemapEntry(int tilemapNum, int tmx, int tmy,uint16_t entry){
	if(tilemapNum < 0 ||tilemapNum >= numTilemaps||tmx<0||tmx>=tilemapTotalWidthTiles||tmy <0 || tmy >= tilemapTotalHeightTiles)
		return;
	tilemaps[tilemapNum][tmy*tilemapTotalWidthTiles+tmx] = entry;
}

void posiAPIDrawTilemap(int tilemapNum, int tmx, int tmy, int tmw, int tmh, int x, int y) {
	if(tilemapNum<0||tilemapNum>=numTilemaps)
		return;
	if(tmx<0||tmx>=tilemapTotalWidthTiles||tmy<0||tmy>=tilemapTotalHeightTiles)
		return;
	if(tmw<0||tmh<0)
		return;
	if(x<0||x>=screenWidth||y<0||y>=screenHeight)
		return;
	auto maxX = x+tmw>=screenWidth?screenWidth-1:x+tmw;
	auto maxY = y+tmh>=screenHeight?screenHeight-1:y+tmh;
	for(auto yy = y;yy<=maxY; yy++) {
		for(auto xx = x;xx<=maxX;xx++){
			auto tmxx = (tmx+(xx-x))%(tilemapTotalWidthTiles*tileSide);
			auto tmyy = (tmy+(yy-y))%(tilemapTotalHeightTiles*tileSide);
			auto tmTileX = tmxx / tileSide;
			auto tmTileY = tmyy / tileSide;
			auto tmPixelX = tmxx % tileSide;
			auto tmPixelY = tmyy % tileSide;
			auto tileNum = tilemaps[tilemapNum][tmTileY*tilemapTotalWidthTiles+tmTileX];
			auto realTileNum = tileNum & 0x3FFF;
			//if(tileNum != 0) {
			//	std::cout<<tmTileX<<" "<<tmTileY<<" "<<tileNum<<" "<<realTileNum<<std::endl;
			//}
			
			if(tileNum & 0x8000) {
				tmPixelX = tileSide - 1 - tmPixelX;
			}
			if(tileNum & 0x4000) {
				tmPixelY = tileSide - 1  - tmPixelY;
			}
			auto pageNum = realTileNum / tilesPerPage;
			auto idRemainder = realTileNum % tilesPerPage;
			auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
			auto tileRow = idRemainder / 16;
			auto tileColumn = idRemainder % 16;
			auto tileStart = pageStart + tileRow * tileRowSize + tileColumn * tileSide;
			auto pixelPos = tileStart + tmPixelY*pixelRowSize+tmPixelX;
			auto color = palette[tiles[pixelPos]];
			posiAPIPutPixel(xx, yy,color);
		}
	}
}