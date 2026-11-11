#pragma once
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"

#define I2C_SENSORS_QTY 2 //Temperatura e umidità (+ io_expander if used)
#define I2C_MUX_ADDRESS 0x70
#define I2C_SENSOR_READ_DELAY_MS 50 //Delay tra letture sensori per evitare self-heating e stabilizzare canale

/* struct i2c_thv_reading {
  float temp;
  float hum;
  float vpd;
}; */

uint8_t i2c_mux_select_bus(uint8_t bus);
void i2c_setup(bool enable_sensors);
void i2c_temp_hum_single_sensor(float &hum, float &temp, uint8_t sensor_bus);
void temp_hum_scan_vpd(float &hum, float &temp, float &vpd);
float calculate_vpd(float temp, float hum);