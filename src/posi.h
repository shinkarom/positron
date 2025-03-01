#pragma once

#include <cstdint>
#include <string>

constexpr auto screenWidth = 256;
constexpr auto screenHeight = 256;
constexpr auto audioFramesPerTick = 44100 / 60;

enum PosiState {POSI_STATE_GAME};

void posi_poweron();
void posi_poweroff();
bool posi_run();
bool posi_load(std::string fileName);
int16_t* posi_audiofeed();
void posi_redraw(uint32_t* buffer);
void posi_change_state(int newState);

bool posi_state_game_run();
