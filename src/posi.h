#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

constexpr auto screenWidth = 256;
constexpr auto screenHeight = 256;
constexpr auto audioSampleRate = 44100;
constexpr auto audioFramesPerTick = audioSampleRate / 60;

constexpr auto tileSide = 8;
constexpr auto tilesPerPage = 256;
constexpr auto pixelsPerPage = tilesPerPage * tileSide * tileSide;
constexpr auto numTilePages = 64;
constexpr auto numTiles = tilesPerPage * numTilePages;
constexpr auto numTilesPixels = pixelsPerPage * numTilePages;

constexpr auto pixelRowSize = 16 * tileSide;
constexpr auto tileRowSize = pixelRowSize * tileSide;

constexpr auto numTilemaps = 32;
constexpr auto tilemapWidthScreens = tileSide;
constexpr auto tilemapHeightScreens = tileSide;
constexpr auto tilemapScreenWidthTiles = screenWidth / tileSide;
constexpr auto tilemapScreenHeightTiles = screenHeight / tileSide;
constexpr auto tilemapScreenTotalTiles = tilemapScreenWidthTiles * tilemapScreenHeightTiles;
constexpr auto tilemapTotalWidthTiles = tilemapScreenWidthTiles * tilemapWidthScreens;
constexpr auto tilemapTotalHeightTiles = tilemapScreenHeightTiles * tilemapHeightScreens;
constexpr auto tilemapTotalTiles = tilemapTotalWidthTiles * tilemapTotalHeightTiles;
constexpr auto tilemapTotalBytes = tilemapTotalTiles * 2;

constexpr auto numInputButtons = 12;
constexpr auto numAudioChannels = 8;

extern bool isFileLoaded;

std::optional<std::vector<uint8_t>> dbLoadByName(const std::string& type, const std::string& name);
std::optional<std::vector<uint8_t>> dbLoadByNumber(const std::string& type, int number);
bool dbTryConnect(const std::string& fileName);
void dbDisconnect();

void luaInit();
void luaClear();
bool luaLoad();
void luaDeinit();
bool luaCallTick();
bool luaReset();
bool luaEvalMain(std::string code);

enum PosiState {POSI_STATE_EMPTY, POSI_STATE_GAME};

void posiPoweron();
void posiPoweroff();
bool posiRun();
bool posiLoad(std::string fileName);
void posiClear();
bool posiReset();
void posiUnload();
int16_t* posiAudiofeed();
void posiChangeState(int newState);

void gpuInit();
void gpuLoad();
void gpuClear();
void gpuReset();
uint32_t* gpuGetBuffer();
void posiRedraw(uint32_t* buffer);
void posiPutPixel(int x, int y, uint32_t color);
void posiAPICls(uint32_t color);
uint32_t posiAPIGetPixel(int x, int y);
void posiAPIPutPixel(int x, int y, uint32_t color);
uint32_t gpuGetTilePagePixel(int pageNum, int x, int y);
uint32_t gpuGetTilePixel(int tileNum, int x, int y);
void posiAPIDrawSprite(int id, int w, int h, int x, int y, bool flipHorz, bool flipVert);
void posiAPIDrawTilemap(int tilemapNum, int tmx, int tmy, int tmw, int tmh, int x, int y);
void posiAPIDrawLine(int x1, int y1, int x2, int y2, uint32_t color);
void posiAPIDrawRect(int x1, int y1, int x2, int y2, uint32_t color) ;
void posiAPIDrawFilledRect(int x1, int y1, int x2, int y2, uint32_t color);
void posiAPIDrawCircle(int centerX, int centerY, int radius, uint32_t color);
void posiAPIDrawFilledCircle(int centerX, int centerY, int radius, uint32_t color);
void posiAPIDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) ;
void posiAPIDrawFilledTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
int posiAPIDrawText(const std::string& text, int x, int y, bool proportional, uint32_t color,int start);
uint16_t posiAPIGetTilemapEntry(int tilemapNum, int tmx, int tmy);
void posiAPISetTilemapEntry(int tilemapNum, int tmx, int tmy, uint16_t entry);

void apuInit();
void apuClearBuffer();
void apuClear();
void apuProcess();
void apuReset();
bool apuLoadFile(std::vector<uint8_t>& file);
void posiAPISetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter,float value);
float posiAPIGetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter);
void posiAPISetGlobalParameter(int channelNumber,uint8_t parameter, float value);
float posiAPIGetGlobalParameter(int channelNumber,uint8_t parameter);
void posiAPINoteOn(int channelNumber,uint8_t note, uint8_t velocity);
void posiAPINoteOff(int channelNumber, uint8_t note);
void posiAPISetSustain(int channelNumber,bool enable);
void posiAPISetModWheel(int channelNumber,uint8_t wheel);
void posiAPISetPitchBend(int channelNumber,uint16_t value);
void posiAPIReleaseAll(int channelNumber);

void posiUpdateButton(int buttonNumber, bool state);
bool API_isPressed(int buttonNumber);
bool API_isJustPressed(int buttonNumber);
bool API_isJustReleased(int buttonNumber);

bool posiStateGameRun();

int16_t floatToInt16(float sample);
float int16ToFloat(int16_t sample);