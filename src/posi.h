#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

constexpr auto screenWidth = 224;
constexpr auto screenHeight = 224;
constexpr auto audioSampleRate = 44100;
constexpr auto audioFramesPerTick = audioSampleRate / 60;

constexpr auto tileSide = 8;
constexpr auto tilesPerPage = 256;
constexpr auto pixelsPerPage = tilesPerPage * tileSide * tileSide;
constexpr auto numTilePages = 16;
constexpr auto numTiles = tilesPerPage * numTilePages;
constexpr auto numTilesPixels = pixelsPerPage * numTilePages;

constexpr auto pixelRowSize = 16 * tileSide;
constexpr auto tileRowSize = pixelRowSize * tileSide;

constexpr auto numTilemaps = 8;
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

std::optional<std::vector<uint8_t>> dbLoadByName(std::string tableName, std::string name);
std::optional<std::vector<uint8_t>> dbLoadByNumber(std::string tableName, int number);
bool dbTryConnect(std::string fileName);
void dbDisconnect();

void luaInit();
void luaDeinit();
bool luaCallTick();
bool luaEvalMain(std::string code);

enum PosiState {POSI_STATE_GAME};

void posiPoweron();
void posiPoweroff();
bool posiRun();
bool posiLoad(std::string fileName);
int16_t* posiAudiofeed();
void posiChangeState(int newState);

void gpuInit();
void gpuLoad();
void posiRedraw(uint32_t* buffer);
void posiPutPixel(int x, int y, uint32_t color);
void posiAPICls(uint32_t color);
void posiAPIPutPixel(int x, int y, uint32_t color);
void posiAPIDrawSprite(int id, int w, int h, int x, int y, bool flipHorz, bool flipVert);
void posiAPIDrawTilemap(int tilemapNum, int tmx, int tmy, int tmw, int tmh, int x, int y);
uint16_t posiAPIGetTilemapEntry(int tilemapNum, int tmx, int tmy);
void posiAPISetTilemapEntry(int tilemapNum, int tmx, int tmy,uint16_t entry);

void apuInit();
void apuProcess();
void apuReset();
bool apuLoadFile(std::vector<uint8_t>& file);
void posiAPITrackStop();
void posiAPITrackPlay(std::string trackName);
void posiUpdateButton(int buttonNumber, bool state);
bool API_isPressed(int buttonNumber);
bool API_isJustPressed(int buttonNumber);
bool API_isJustReleased(int buttonNumber);

bool posiStateGameRun();
