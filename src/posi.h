#pragma once

#include <cstdint>

constexpr auto screenWidth = 320;
constexpr auto screenHeight = 320;
constexpr auto audioFramesPerTick = 44100 / 60;

void posi_poweron();
void posi_poweroff();
void posi_run();
void posi_load();
void posi_save();
int16_t* posi_audiofeed();
void posi_redraw(uint32_t* buffer);
