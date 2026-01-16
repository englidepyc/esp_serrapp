//TODO: Testare!
//TODO: Trovare gli indirizzi
//TODO: Capire dove mettere i pin, aggiornare gli indirizzi
/*Letture I2C di Temperatura e Umidità grazie al mux TCA9548A (I2C mux) dai sensori AHT20 e BMP280 sulla board*/

#include <Wire.h>
#include <Adafruit_AHTX0.h> // Library for AHT20
#include <Adafruit_BMP280.h> // Library for BMP280
#include "pins.h"

Adafruit_AHTX0 aht;
sensors_event_t humidity, temp;

//TCA9548A
uint8_t i2c_mux_select_bus(uint8_t bus){
  if (bus > 7) return; // Protezione: il multiplexer ha solo 8 canali (0-7)
  
  Wire.beginTransmission(0x70); //collegare a low a0-2
  Wire.write(1 << bus);
  uint8_t error_code = Wire.endTransmission();
  return error_code;
}

//inzializza tutti i sensori sul mux, da chiamare al setup
void i2c_setup() {
  Wire.begin();

  //controlla che il mux i2c è ok
  (i2c_mux_select_bus(0) != 0) ? Serial.print("[ERRORE!] Mux i2c non trovato/funziona!") : Serial.print(" Mux i2c OK!");

  for (uint8_t i = 0; i < 4; i++) {
    i2c_mux_select_bus(i); // Apri il canale i
    if (!aht.begin()) {
      Serial.printf("Sensore sul bus %d non trovato!\n", i);
    } else {
      Serial.printf("Sensore sul bus %d inizializzato.\n", i);
    }
  }
}

//you can access the updated "humidity.relative_humidity" and "temp.temperature" values after
void i2c_update_temperature_hum(){
    aht.getEvent(&humidity,&temp);
}