#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

constexpr auto screenWidth = 256;
constexpr auto screenHeight = 224;
constexpr auto audioFramesPerTick = 44100 / 60;

constexpr auto tileSide = 8;
constexpr auto tilesPerPage = 256;
constexpr auto numTilePages = 256;
constexpr auto numTiles = tilesPerPage * numTilePages;
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

void gpuLoad();
void posiRedraw(uint32_t* buffer);
void posiAPICls(uint32_t color);
void posiAPIPutPixel(int x, int y, uint32_t color);
void posiAPIDrawSprite(int id, int w, int h, int x, int y, bool flipHorz, bool flipVert);

void posiUpdateButton(int buttonNumber, bool state);
bool API_isPressed(int buttonNumber);
bool API_isJustPressed(int buttonNumber);
bool API_isJustReleased(int buttonNumber);

bool posiStateGameRun();
