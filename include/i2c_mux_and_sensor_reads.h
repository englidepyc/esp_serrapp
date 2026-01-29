#pragma once
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"

#define I2C_SENSORS_QTY 2 //Temperatura e umidità
#define I2C_MUX_ADDRESS 0x70

/* struct i2c_thv_reading {
  float temp;
  float hum;
  float vpd;
}; */

uint8_t i2c_mux_select_bus(uint8_t bus);
void i2c_setup();
void i2c_temp_hum_vpd_scan(float &hum, float &temp, float &vpd, uint8_t sensor_bus);
float calculate_vpd(float temp, float hum);