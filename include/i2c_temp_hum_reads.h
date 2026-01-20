#pragma once
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"

struct i2c_th_reading {
  float temp;
  float hum;
};

uint8_t i2c_mux_select_bus(uint8_t bus);
void i2c_setup();
i2c_th_reading i2c_update_temperature_hum();