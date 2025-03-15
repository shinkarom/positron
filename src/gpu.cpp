#include "posi.h"

#include <vector>
#include <iostream>
#include <cstring>

std::vector<uint32_t> frameBuffer(screenWidth * screenHeight);
std::vector<uint32_t> tiles(numTilesPixels);
std::vector<uint16_t> tilemaps[numTilemaps];

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
	for(auto i = 0; i<numTilemaps; i++) {
		tilemaps[i].resize(tilemapTotalTiles);
	}
}

void gpuLoad() {
	for(auto i = 0; i< numTilePages; i++) {
		auto x = dbLoadByNumber("tiles", i);
		if(!x || x->size() != pixelsPerPage*4) continue;
		auto outputStart = tiles.data() + i * (pixelsPerPage);
		memcpy(outputStart, x->data(), pixelsPerPage*4);
	}
	for(auto i = 0; i<numTilemaps; i++) {
		auto x = dbLoadByNumber("tilemaps", i);
		if(!x || x->size() != tilemapTotalBytes) continue;
		memcpy(tilemaps[i].data(), x->data(), tilemapTotalBytes);
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
	if(tmw<0||tmw>screenWidth||tmh<0||tmh>screenHeight)
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
			auto color = tiles[pixelPos];
			posiAPIPutPixel(xx, yy,color);
		}
	}
}