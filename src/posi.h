#pragma once

#include <cstdint>
#include <string>

constexpr auto maxScreenWidth = 256;
constexpr auto maxScreenHeight = 256;
constexpr auto screenWidth = 256;
constexpr auto screenHeight = 256;
constexpr auto audioFramesPerTick = 44100 / 60;

constexpr auto numInputButtons = 32;

enum PosiState {POSI_STATE_GAME};

void posiPoweron();
void posiPoweroff();
bool posiRun();
bool posiLoad(std::string fileName);
int16_t* posiAudiofeed();
void posiChangeState(int newState);

void posiRedraw(uint32_t* buffer);
void posiAPICls(uint32_t color);
void posiAPIPutPixel(uint8_t depth, int x, int y, uint32_t color);

void posiUpdateButton(int buttonNumber, bool state);
bool API_isPressed(int buttonNumber);
bool API_isJustPressed(int buttonNumber);
bool API_isJustReleased(int buttonNumber);

bool posiStateGameRun();
