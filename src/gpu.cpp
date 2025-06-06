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


uint32_t gpuGetTilePagePixel(int pageNum, int x, int y) {
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

void gpuSetTilePagePixel(int pageNum, int x, int y, uint32_t color) {
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

uint32_t gpuGetTilePixel(int tileNum, int x, int y) {
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

void gpuSetTilePixel(int tileNum, int x, int y, uint32_t color) {
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

uint32_t* gpuGetBuffer() {
	return frameBuffer.data();
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
	if(tileRow + w > 16){
		w = 16 - tileRow;
	}
	if(tileColumn + h > 16) {
		h = 16 - tileColumn;
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
			auto tileN = (tileR*16)+tileC;
			//std::cout<<id<<" | "<<idRemainder<<" | "<<tileRow<<" "<<tileColumn<<" | "<<tileR<<" "<<tileC<<" | "<<tileN<<std::endl;
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
	//x1 = std::clamp(x1,0,screenWidth-1);
	//y1 = std::clamp(y1,0,screenHeight-1);
	//x2 = std::clamp(x2,0,screenWidth-1);
	//y2 = std::clamp(y2,0,screenHeight-1);
	
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
    //minX = std::clamp(minX, 0, screenWidth - 1);
    //minY = std::clamp(minY, 0, screenHeight - 1);
    //maxX = std::clamp(maxX, 0, screenWidth - 1);
    //maxY = std::clamp(maxY, 0, screenHeight - 1);

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
    //minX = std::clamp(minX, 0, screenWidth - 1);
    //minY = std::clamp(minY, 0, screenHeight - 1);
    //maxX = std::clamp(maxX, 0, screenWidth - 1);
    //maxY = std::clamp(maxY, 0, screenHeight - 1);

    // Iterate through each row within the rectangle and draw a horizontal line
    for (int y = minY; y <= maxY; ++y) {
        posiAPIDrawLine(minX, y, maxX, y, color);
    }
}


void posiAPIDrawCircle(int centerX, int centerY, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        posiAPIPutPixel(centerX + x, centerY + y, color);
        posiAPIPutPixel(centerX - x, centerY + y, color);
        posiAPIPutPixel(centerX + x, centerY - y, color);
        posiAPIPutPixel(centerX - x, centerY - y, color);
        posiAPIPutPixel(centerX + y, centerY + x, color);
        posiAPIPutPixel(centerX - y, centerY + x, color);
        posiAPIPutPixel(centerX + y, centerY - x, color);
        posiAPIPutPixel(centerX - y, centerY - x, color);

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

    // Draw the three lines of the triangle
    posiAPIDrawLine(x1, y1, x2, y2, color);
    posiAPIDrawLine(x2, y2, x3, y3, color);
    posiAPIDrawLine(x3, y3, x1, y1, color);
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

int posiAPIDrawText(const std::string& text, int x, int y, bool proportional, uint32_t color,int start){
	if(x <0 || y < 0 || start < 0 || start >= numTiles) {
		return 0;
	}
	auto textWidth = 0;
	auto textHeight = 0;
	auto charStartX = x;
	auto charStartY = y;
	
	auto lineHeight = 0;
	
	for(char ch : text) {
		if(charStartX>=screenWidth || charStartY >= screenHeight) { break; }

		if(ch == '\n') {
			charStartX = x;
			auto g = proportional ? lineHeight+1 : tileSide;
			charStartY += g;
			textHeight += g;
			continue;
		} else if(ch == '\t') {
			constexpr auto tabStopWidth = 4 * tileSide;
			auto addAmount = tabStopWidth - (charStartX % tabStopWidth);
			charStartX += addAmount;
			textWidth += addAmount;
			continue;
		} else if (ch < ' '|| ch > 127) { continue; }
		
		auto tileId = start + ch - 32;
		if(tileId >= numTiles) {
			continue;
		}
		auto tileStart = tileId * tileSide * tileSide;
		
		//get bounding box
		auto horzMin = 0;
		auto horzMax = -1;
		if(proportional) {
			for (auto tileY = 0; tileY < tileSide; tileY++) {
				for (auto tileX = 0; tileX < tileSide; tileX++) {
					auto tilePixelColor = tiles[tileStart+tileY*tileSide+tileX];
					if(tilePixelColor != 0x00000000) {
						if(horzMin == 0 || tileX < horzMin) {horzMin = tileX;}
						if(tileX > horzMax) {horzMax = tileX;}
						if(tileY > lineHeight) {lineHeight = tileY;}
					}
				}
			}	
		} else {
			horzMax = tileSide - 1;
			lineHeight = tileSide - 1;
		}
		if(horzMax == -1) {
			auto g = proportional ? 4 : tileSide;
			charStartX += g;
			textWidth += g;
			continue;
		} 
		for (auto tileY = 0; tileY <= lineHeight; tileY++) {
			for (auto tileX = horzMin; tileX <= horzMax; tileX++) {
				auto xxxx = tileX - horzMin;
				auto c = tiles[tileStart+tileY*tileSide+tileX];
				if(c != 0x00000000) {
					posiAPIPutPixel(charStartX+xxxx, charStartY+tileY, color);
				}
			}
		}
		auto g = proportional ? horzMax - horzMin + 1 + 1 : tileSide;
		charStartX += g;
		textWidth += g;
		
	}
	return textWidth;
}