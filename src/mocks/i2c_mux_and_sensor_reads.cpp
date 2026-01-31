constexpr static const char* M_I2C_TEMP_HUM_READS_H_TAG = "MOCK/I2C_TEMP_HUM_READS.CPP";
/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"
#include "i2c_mux_and_sensor_reads.h"

//TCA9548A
uint8_t i2c_mux_select_bus(uint8_t bus){
  if (bus >= I2C_SENSORS_QTY) {ESP_LOGW(M_I2C_TEMP_HUM_READS_H_TAG,"Error, out of range i2c bus!"); return 1;}
  else return 0;
}

//inzializza tutti i sensori sul mux, da chiamare al setup
void i2c_setup(bool enable_sensors) {
  for (uint8_t i = 0; i < I2C_SENSORS_QTY; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
      ESP_LOGD(M_I2C_TEMP_HUM_READS_H_TAG,"Mock i2c mux ok, mock sensore setup %d inizializzato.\n", i);  
  }
}

//returns a single measured relative humidity and temperature out of an aht sensor
void i2c_temp_hum_vpd_scan(float &hum, float &temp, float &vpd, uint8_t sensor_bus){ 
  temp = (float)random(5, 41);              // 5 to 40 °C
  hum = (float)random(0, 101);              // 0 to 100 %
  vpd = (float)random(10, 35) / 10.0;       // 1.0 to 3.5 kPa (typical VPD range for southern Italy summer)
}

float calculate_vpd(float temp, float hum){
  // VPD = (1 - RH/100) * SVP
  // SVP (Saturation Vapor Pressure) = 0.6108 * exp((17.27 * T) / (T + 237.3))
  float svp = 0.6108 * exp((17.27 * temp) / (temp + 237.3));
  float vpd = (1.0 - hum / 100.0) * svp;
  return vpd;
}