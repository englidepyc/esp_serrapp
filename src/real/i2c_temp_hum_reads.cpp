constexpr static const char* I2C_TEMP_HUM_READS_H_TAG = "I2C_TEMP_HUM_READS.CPP";
//TODO: Testare!
//TODO: Sistemare funzioni di ritorno
/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"
#include "i2c_temp_hum_reads.h"

static Adafruit_AHTX0 _aht;
static sensors_event_t _humidity, _temp;

//Ritorna codice di errore
uint8_t i2c_mux_select_bus(uint8_t bus){
  if (bus > 7) {ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG,"Error, i2c bus non existent!"); return 1;} // Protezione: il multiplexer ha solo 8 canali (0-7)
  
  Wire.beginTransmission(0x70); //collegare a low a0-2
  Wire.write(1 << bus);
  uint8_t error_code = Wire.endTransmission();
  return error_code;
}

//inzializza tutti i sensori sul mux, da chiamare al setup
void i2c_setup() {
  Wire.begin();

  //controlla che il mux i2c è ok
  if (i2c_mux_select_bus(0) != 0) {
    ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG, "Mux i2c non funziona!");
  } else {
    ESP_LOGD(I2C_TEMP_HUM_READS_H_TAG, "Mux i2c OK");
  }

  for (uint8_t i = 0; i < 4; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
    if (!_aht.begin()) {
      ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG, "Sensore sul bus %d non trovato!", i);
    } else {
      ESP_LOGD(I2C_TEMP_HUM_READS_H_TAG, "Sensore sul bus %d inizializzato", i);
    }
  }

  ESP_LOGI(I2C_TEMP_HUM_READS_H_TAG,"Inizializzazione sensori i2c completata!");
}

//returns a single measured relative humidity and temperature out of an aht sensor
i2c_th_reading i2c_update_temperature_hum(){
  i2c_th_reading reading;
  _aht.getEvent(&_humidity,&_temp);
  
  reading.hum = _humidity.relative_humidity;
  reading.temp = _temp.temperature;

  return reading;
}