#include "posi.h"

#include <iostream>
#include <cstring>
#include <array>
#include <utility>

std::array<uint32_t, screenWidth * screenHeight> frameBuffer;
std::array<uint32_t, numTilesPixels> tiles;
std::array<uint16_t, tilemapTotalTiles> tilemaps[numTilemaps];

void posiPutPixel(int x, int y, uint32_t color) {
	if(color == 0 || x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
		return;
	}
	if(!(color&0xFF000000)) return;
	frameBuffer[y * screenWidth + x] = color;
}

void posiAPICls(uint32_t color) {
	color = 0xFF000000 | color;
	for(int j = 0; j < screenHeight; j++) {
		for(int i = 0; i < screenWidth; i++) {
			posiPutPixel(i, j, color);
		}
	}
}

uint32_t posiAPIGetPixel(int x, int y) {
	if(x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) {
		return 0xFF000000;
	}
	return frameBuffer[y*screenWidth+x];
}

void posiAPIPutPixel(int x, int y, uint32_t color) {
	posiPutPixel(x, y, color);
}


uint32_t posiAPIGetTilePagePixel(int pageNum, int x, int y) {
	if(x < 0 || x >= 128 || y < 0 || y >= 128 || pageNum < 0 || pageNum >= numTilePages) {
		return 0xFF000000;
	}
	auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
	auto tileRow = y / 8;
	auto tileCol = x / 8;
	auto pxRow = y % 8;
	auto pxCol = x % 8;
	auto tileNum = tileRow * 16 + tileCol;
	auto pxAddr = pxRow * 8 + pxCol;
	auto pxAddress = pageStart + (tileNum * 64) + pxAddr;
	return tiles[pxAddress];
}

void posiAPISetTilePagePixel(int pageNum, int x, int y, uint32_t color) {
	if(x < 0 || x >= 128 || y < 0 || y >= 128 || pageNum < 0 || pageNum >= numTilePages) {
		return;
	}
	auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
	auto tileRow = y / 8;
	auto tileCol = x / 8;
	auto pxRow = y % 8;
	auto pxCol = x % 8;
	auto tileNum = tileRow * 16 + tileCol;
	auto pxAddr = pxRow * 8 + pxCol;
	auto pxAddress = pageStart + (tileNum * 64) + pxAddr;
	tiles[pxAddress] = color | 0xFF000000;
}

uint32_t posiAPIGetTilePixel(int tileNum, int x, int y) {
	if(x < 0 || x >= tileSide || y < 0 || y >= tileSide || tileNum < 0 || tileNum >= numTiles) {
		return 0xFF000000;
	}
	auto pageNum = tileNum / tilesPerPage;
	auto idRemainder = tileNum % tilesPerPage;
	auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
	auto tileStart = pageStart + idRemainder*64;
	auto pxAddr = tileStart + (y * tileSide) + x;
	return tiles[pxAddr];
	
}

void posiAPISetTilePixel(int tileNum, int x, int y, uint32_t color) {
	if(x < 0 || x >= tileSide || y < 0 || y >= tileSide || tileNum < 0 || tileNum >= numTiles) {
		return;
	}
	auto pageNum = tileNum / tilesPerPage;
	auto idRemainder = tileNum % tilesPerPage;
	auto pageStart = pageNum * tilesPerPage*tileSide*tileSide;
	auto tileStart = pageStart + idRemainder*64;
	auto pxAddr = tileStart + (y * tileSide) + x;
	tiles[pxAddr] = color | 0xFF000000;;
}

void posiRedraw(uint32_t* buffer) {	
	memcpy(buffer, frameBuffer.data(),screenHeight*screenWidth*4);
}

void gpuInit() {
    gpuClear();
}

void loadTilePages() {
	for(auto i = 0; i< numTilePages; i++) {
		auto x = dbLoadByNumber("tiles", i);
		if(!x || x->size() != pixelsPerPage*4) continue;
		auto outputStart = tiles.data() + i * (pixelsPerPage);
		uint32_t* dest = (uint32_t*)outputStart;
		uint32_t* pixels = (uint32_t*)(x->data());
		for (size_t i = 0; i < pixelsPerPage; i++) {
			dest[i] = pixels[i];
		}	
	}	
}

void loadTilemaps() {
	for(auto i = 0; i<numTilemaps; i++) {
		auto x = dbLoadByNumber("tilemap", i);
		if(!x || x->size() != tilemapTotalBytes) continue;
		memcpy(tilemaps[i].data(), x->data(), tilemapTotalBytes);
	}
}

void gpuClear() {
	frameBuffer.fill(0);
	tiles.fill(0);
	for(int j = 0; j < numTilemaps; j++) {
		tilemaps[j].fill(0);
	}
}

void gpuReset() {
	gpuClear();
	gpuLoad();
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
	for(auto yy = 0; yy < h*tileSide; yy++) {
		auto posY = flipVert ? y + h * tileSide - yy : y+yy;
		auto divY = yy / tileSide;
		auto remY = yy % tileSide;
		auto tileR = tileRow + divY;
		for(auto xx = 0; xx < w*tileSide; xx++){
			auto posX = flipHorz ? x + w * tileSide - xx : x+xx;
			auto divX = xx / tileSide;
			auto remX = xx % tileSide;
			auto tileC = tileColumn + divX;
			auto tileN = idRemainder +(tileR*16)+tileC;
			auto tileStart = pageStart + tileN*64;
			auto colorPos = tileStart + remY*tileSide + remX;
			auto pixelColor = tiles[colorPos];
			posiPutPixel(posX,posY, pixelColor);
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
	if(tmx<0||tmx>=tilemapTotalWidthTiles*tileSide||tmy<0||tmy>=tilemapTotalHeightTiles*tileSide)
		return;
	if(tmw<0||tmh<0)
		return;
	if(x<0||x>=screenWidth||y<0||y>=screenHeight)
		return;
	auto maxX = x+tmw>=screenWidth?screenWidth-1:x+tmw-1;
	auto maxY = y+tmh>=screenHeight?screenHeight-1:y+tmh-1;
	for(auto yy = y;yy<=maxY; yy++) {
		auto tmyy = (tmy+(yy-y))%(tilemapTotalHeightTiles*tileSide);
		auto tmTileY = tmyy / tileSide;
		auto tmPixelY = tmyy % tileSide;
		for(auto xx = x;xx<=maxX;xx++){
			auto tmxx = (tmx+(xx-x))%(tilemapTotalWidthTiles*tileSide);
			auto tmTileX = tmxx / tileSide;
			auto tmPixelX = tmxx % tileSide;
			auto tileAddr = tmTileY*tilemapTotalWidthTiles+tmTileX;
			auto tileNum = tilemaps[tilemapNum][tileAddr];
			auto realTileNum = tileNum & 0x3FFF;
			if(realTileNum >= numTiles) {
				continue;
			}
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
			auto tileStart = pageStart + idRemainder * 64;
			auto pixelPos = tileStart + tmPixelY*tileSide+tmPixelX;
			auto color = tiles[pixelPos];
			posiPutPixel(xx, yy,color);
		}
	}
}