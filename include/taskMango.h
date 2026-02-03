#pragma once
#include <esp_attr.h>
#include "types.h"

constexpr float K_WATER = 4.0; //percentuale al secondo
void updateCalcWaterLevel(float& waterLevel_percentage, int duration_s);