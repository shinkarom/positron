#pragma once

#include <cstdint>
#include <string>

constexpr auto maxScreenWidth = 256;
constexpr auto maxScreenHeight = 256;
constexpr auto screenWidth = 256;
constexpr auto screenHeight = 256;
constexpr auto audioFramesPerTick = 44100 / 60;

enum PosiState {POSI_STATE_GAME};

void posiPoweron();
void posiPoweroff();
bool posiRun();
bool posiLoad(std::string fileName);
int16_t* posiAudiofeed();
void posiRedraw(uint32_t* buffer);
void posiChangeState(int newState);
void posiAPICls(uint32_t color);

bool posiStateGameRun();
