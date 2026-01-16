/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/

/* #include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h" */
#pragma once
#include <Arduino.h>


static const char* TAG = "MOCK/I2C_TEMP_HUM_READS.H";

struct Humidity{
  float relative_humidity;
};

struct Temperature{
  float temperature;
};

Temperature temp;
Humidity humidity;

//TCA9548A
void i2c_mux_select_bus(uint8_t bus){
  if (bus > 7) return; // Protezione: il multiplexer ha solo 8 canali (0-7)
}

//inzializza tutti i sensori sul mux, da chiamare al setup
void i2c_setup() {
  for (uint8_t i = 0; i < 4; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
      ESP_LOGD(TAG,"Mock i2c mux ok, mock sensore setup %d inizializzato.\n", i);  
  }
}

//you can access the updated "humidity.relative_humidity" and "temp.temperature" values after
void i2c_update_temperature_hum(){
  temp.temperature = (float)random(5, 41);              // 5 to 40 °C
  humidity.relative_humidity = (float)random(0, 101);   // 0 to 100 %
}