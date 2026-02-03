#pragma once
#include <esp_attr.h>

constexpr float K_WATER = 4.0; //percentuale al secondo

void updateCalcWaterLevel(float& waterLevel_percentage, int duration_s);