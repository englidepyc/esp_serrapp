#pragma once
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"

/* struct i2c_thv_reading {
  float temp;
  float hum;
  float vpd;
}; */

uint8_t i2c_mux_select_bus(uint8_t bus);
void i2c_setup();
void i2c_temp_hum_vpd_scan(float &hum, float &temp, float &vpd);
float calculate_vpd(float temp, float hum);