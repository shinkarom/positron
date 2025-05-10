#pragma once

#include <string>

void posiSDLInit();
bool posiSDLTick();
void posiSDLCountTime();
void posiSDLDestroy();
bool posiSDLLoadFile(std::string fileName);