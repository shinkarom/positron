#include "posi.h"

#include <iostream>
#include <cstring>
#include <array>
#include <utility>
#include <algorithm>

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

void posiAPIDrawLine(int x1, int y1, int x2,int y2, uint32_t color) {
	x1 = std::clamp(x1,0,screenWidth-1);
	y1 = std::clamp(y1,0,screenHeight-1);
	x2 = std::clamp(x2,0,screenWidth-1);
	y2 = std::clamp(y2,0,screenHeight-1);
	
	int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        posiAPIPutPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err = err - dy;
            x1 = x1 + sx;
        }
        if (e2 < dx) {
            err = err + dx;
            y1 = y1 + sy;
        }
    }
}

void posiAPIDrawRect(int x1, int y1, int x2, int y2, uint32_t color) {
    int minX = std::min(x1, x2);
    int minY = std::min(y1, y2);
    int maxX = std::max(x1, x2);
    int maxY = std::max(y1, y2);

    // Clamp the rectangle coordinates
    minX = std::clamp(minX, 0, screenWidth - 1);
    minY = std::clamp(minY, 0, screenHeight - 1);
    maxX = std::clamp(maxX, 0, screenWidth - 1);
    maxY = std::clamp(maxY, 0, screenHeight - 1);

    // Draw the four lines of the rectangle
    posiAPIDrawLine(minX, minY, maxX, minY, color); // Top
    posiAPIDrawLine(maxX, minY, maxX, maxY, color); // Right
    posiAPIDrawLine(maxX, maxY, minX, maxY, color); // Bottom
    posiAPIDrawLine(minX, maxY, minX, minY, color); // Left
}

void posiAPIDrawFilledRect(int x1, int y1, int x2, int y2, uint32_t color) {
    int minX = std::min(x1, x2);
    int minY = std::min(y1, y2);
    int maxX = std::max(x1, x2);
    int maxY = std::max(y1, y2);

    // Clamp the rectangle coordinates
    minX = std::clamp(minX, 0, screenWidth - 1);
    minY = std::clamp(minY, 0, screenHeight - 1);
    maxX = std::clamp(maxX, 0, screenWidth - 1);
    maxY = std::clamp(maxY, 0, screenHeight - 1);

    // Iterate through each row within the rectangle and draw a horizontal line
    for (int y = minY; y <= maxY; ++y) {
        posiAPIDrawLine(minX, y, maxX, y, color);
    }
}


void posiAPIDrawCircle(int centerX, int centerY, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto drawPixel = [&](int px, int py) {
        int clampedX = std::clamp(px, 0, screenWidth - 1);
        int clampedY = std::clamp(py, 0, screenHeight - 1);
        posiAPIPutPixel(clampedX, clampedY, color);
    };

    while (x <= y) {
        drawPixel(centerX + x, centerY + y);
        drawPixel(centerX - x, centerY + y);
        drawPixel(centerX + x, centerY - y);
        drawPixel(centerX - x, centerY - y);
        drawPixel(centerX + y, centerY + x);
        drawPixel(centerX - y, centerY + x);
        drawPixel(centerX + y, centerY - x);
        drawPixel(centerX - y, centerY - x);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void posiAPIDrawFilledCircle(int centerX, int centerY, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto drawHorizontalLine = [&](int yCoord, int xStart, int xEnd) {
        int clampedY = std::clamp(yCoord, 0, screenHeight - 1);
        int clampedXStart = std::clamp(xStart, 0, screenWidth - 1);
        int clampedXEnd = std::clamp(xEnd, 0, screenWidth - 1);
        if (clampedXStart <= clampedXEnd) {
            posiAPIDrawLine(clampedXStart, clampedY, clampedXEnd, clampedY, color);
        } else {
            posiAPIDrawLine(clampedXEnd, clampedY, clampedXStart, clampedY, color);
        }
    };

    while (x <= y) {
        drawHorizontalLine(centerY + y, centerX - x, centerX + x);
        drawHorizontalLine(centerY - y, centerX - x, centerX + x);

        if (x != y) {
            drawHorizontalLine(centerY + x, centerX - y, centerX + y);
            drawHorizontalLine(centerY - x, centerX - y, centerX + y);
        }

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void posiAPIDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    // Clamp the coordinates
    int cX1 = std::clamp(x1, 0, screenWidth - 1);
    int cY1 = std::clamp(y1, 0, screenHeight - 1);
    int cX2 = std::clamp(x2, 0, screenWidth - 1);
    int cY2 = std::clamp(y2, 0, screenHeight - 1);
    int cX3 = std::clamp(x3, 0, screenWidth - 1);
    int cY3 = std::clamp(y3, 0, screenHeight - 1);

    // Draw the three lines of the triangle
    posiAPIDrawLine(cX1, cY1, cX2, cY2, color);
    posiAPIDrawLine(cX2, cY2, cX3, cY3, color);
    posiAPIDrawLine(cX3, cY3, cX1, cY1, color);
}

void posiAPIDrawFilledTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    // Sort vertices by y-coordinate
    std::vector<std::pair<int, int>> vertices = {{y1, x1}, {y2, x2}, {y3, x3}};
    std::sort(vertices.begin(), vertices.end());

    int y_min = vertices[0].first;
    int y_mid = vertices[1].first;
    int y_max = vertices[2].first;
    int x_min = vertices[0].second;
    int x_mid = vertices[1].second;
    int x_max = vertices[2].second;

    auto interpolate = [](int y, int y1, int x1, int y2, int x2) {
        if (y2 == y1) return x1;
        return static_cast<int>(static_cast<double>(x1) + (static_cast<double>(y - y1) / (y2 - y1)) * (x2 - x1));
    };

    for (int y = y_min; y <= y_max; ++y) {
        int x_left, x_right;

        if (y <= y_mid) {
            x_left = interpolate(y, y_min, x_min, y_mid, x_mid);
            x_right = interpolate(y, y_min, x_min, y_max, x_max);
        } else {
            x_left = interpolate(y, y_mid, x_mid, y_max, x_max);
            x_right = interpolate(y, y_min, x_min, y_max, x_max);
        }

        if (x_left > x_right) std::swap(x_left, x_right);

        for (int x = x_left; x <= x_right; ++x) {
            if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
                posiAPIPutPixel(x, y, color);
            }
        }
    }
}