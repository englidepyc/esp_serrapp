constexpr static const char* M_I2C_TEMP_HUM_READS_H_TAG = "MOCK/I2C_TEMP_HUM_READS.CPP";
/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"
#include "i2c_temp_hum_reads.h"

//TCA9548A
uint8_t i2c_mux_select_bus(uint8_t bus){
  if (bus > 7) {ESP_LOGW(M_I2C_TEMP_HUM_READS_H_TAG,"Error, i2c bus non existent!"); return 1;} // Protezione: il multiplexer ha solo 8 canali (0-7)
  else return 0;
}

//inzializza tutti i sensori sul mux, da chiamare al setup
void i2c_setup() {
  for (uint8_t i = 0; i < 4; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
      ESP_LOGD(M_I2C_TEMP_HUM_READS_H_TAG,"Mock i2c mux ok, mock sensore setup %d inizializzato.\n", i);  
  }
}

//returns a single measured relative humidity and temperature out of an aht sensor
i2c_th_reading i2c_update_temperature_hum(){
  i2c_th_reading reading;
  
  reading.temp = (float)random(5, 41);              // 5 to 40 °C
  reading.hum = (float)random(0, 101);              // 0 to 100 %
  
  return reading;
}