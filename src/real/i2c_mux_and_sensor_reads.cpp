constexpr static const char* I2C_TEMP_HUM_READS_H_TAG = "I2C_TEMP_HUM_READS.CPP";
//TODO: Testare!
/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/
#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"
#include "i2c_mux_and_sensor_reads.h"

static Adafruit_AHTX0 _aht;
static sensors_event_t _humidity, _temp;

//Ritorna codice di errore
uint8_t i2c_mux_select_bus(uint8_t bus){
  if (bus >= I2C_SENSORS_QTY) {ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG,"Error, out of range i2c bus!"); return 1;} // Protezione: il multiplexer ha solo 8 canali (0-7)
  
  Wire.beginTransmission(I2C_MUX_ADDRESS); //collegare a low a0-2
  Wire.write(1 << bus);
  uint8_t error_code = Wire.endTransmission();
  return error_code;
}

//inzializza la comunicazione i2c e tutti i sensori sul mux, da chiamare al setup
void i2c_setup(bool enable_sensors = false) {
  Wire.begin(SDA_PIN,SCL_PIN);

  //controlla che il mux i2c è ok
  if (i2c_mux_select_bus(0) != 0) {
    ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG, "Mux i2c non funziona!");
  } else {
    ESP_LOGD(I2C_TEMP_HUM_READS_H_TAG, "Mux i2c OK");
  }

  if(enable_sensors){
    for (uint8_t i = 0; i < I2C_SENSORS_QTY; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
    if (!_aht.begin()) {
      ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG, "Sensore sul bus %d non trovato!", i);
    } else {
      ESP_LOGD(I2C_TEMP_HUM_READS_H_TAG, "Sensore sul bus %d inizializzato", i);
    }
  }

  ESP_LOGI(I2C_TEMP_HUM_READS_H_TAG,"Inizializzazione sensori i2c completata!");
  }
  
}

//get a single measured relative humidity and temperature out of an aht sensor
void temp_hum_single_sensor(float &hum, float &temp, uint8_t sensor_bus){
  i2c_mux_select_bus(sensor_bus);
  _aht.getEvent(&_humidity,&_temp);
  
  hum = _humidity.relative_humidity;
  temp = _temp.temperature;
}

void temp_hum_scan_vpd(float &hum, float &temp, float &vpd){
  float sum_hum = 0.0;
  float sum_temp = 0.0;
  uint8_t valid_readings = 0;

  // Leggi tutti i sensori
  for (uint8_t i = 0; i < I2C_SENSORS_QTY; i++) {
    float sensor_hum, sensor_temp;
    temp_hum_single_sensor(sensor_hum, sensor_temp, i);
    
    // Verifica che le letture siano valide
    if (!isnan(sensor_hum) && !isnan(sensor_temp)) {
      sum_hum += sensor_hum;
      sum_temp += sensor_temp;
      valid_readings++;
    }
    
    // Delay per evitare self-heating e stabilizzare il canale I2C
    if (i < I2C_SENSORS_QTY - 1) {
      vTaskDelay(pdMS_TO_TICKS(I2C_SENSOR_READ_DELAY_MS));
    }
  }

  // Calcola la media
  if (valid_readings > 0) {
    hum = sum_hum / valid_readings;
    temp = sum_temp / valid_readings;
    vpd = calculate_vpd(temp, hum);
  } else {
    // Nessuna lettura valida
    hum = NAN;
    temp = NAN;
    vpd = NAN;
    ESP_LOGW(I2C_TEMP_HUM_READS_H_TAG, "Nessuna lettura valida dai sensori");
  }
}

float calculate_vpd(float temp, float hum){
  // VPD = (1 - RH/100) * SVP
  // SVP (Saturation Vapor Pressure) = 0.6108 * exp((17.27 * T) / (T + 237.3))
  float svp = 0.6108 * exp((17.27 * temp) / (temp + 237.3));
  float vpd = (1.0 - hum / 100.0) * svp;
  return vpd;
}