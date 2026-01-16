#include <Arduino.h>
#include "pins.h"

static const char* TAG = "SHIFT_REGISTER.H";
const char* deviceNames[] = {"PUMP", "HEATER", "HEAT_FAN", "HUMIDIFIER", "EXT_FAN", "AMUX0", "AMUX1", "AMUX2"};

#define  ON_R LOW
#define  OFF_R HIGH

enum Device : byte {
   PUMP_Q0     = 0,
   HEATER_Q1   = 1,
   FAN_HEAT_Q2 = 2,
   HUMIF_Q3    = 3,
   FAN_EXT_Q4  = 4,

   AMUX_0_Q5   = 5,
   AMUX_1_Q6   = 6,
   AMUX_2_Q7   = 7,
};


byte current_sr_bits = 0; 

void updateShiftRegister() {
   digitalWrite(LATCH_PIN, LOW);
   shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, current_sr_bits);
   digitalWrite(LATCH_PIN, HIGH);
}

void setAllOutputs(bool state){
   current_sr_bits = (state ? 0b11111111 : 0);
   updateShiftRegister(); 
}

void setDevice(Device dev, bool state) {
   state ? bitSet(current_sr_bits, dev) : bitClear(current_sr_bits, dev);
   updateShiftRegister();

   // Using your ON_R (LOW) define for clarity
   if (state == ON_R) {
     ESP_LOGI(TAG, "%s activated!", deviceNames[dev]);
   } else {
     ESP_LOGD(TAG, "%s deactivated", deviceNames[dev]);
   }
}

void setAmuxChannel(byte channel) {
    
   if (channel > 7){
      ESP_LOGW(TAG, "Canale mux analogico specificato non valido!\n");
      return;
    } 
    
    // 1. Puliamo i bit 5, 6 e 7 della variabile current_sr_bits (mettiamoli a 0)
    // Usiamo una maschera: 0b00011111 (0x1F) mantiene i primi 5 bit e azzera gli ultimi 3
    current_sr_bits &= 0b00011111;

    // 2. Prendiamo il numero del canale e lo spostiamo a sinistra di 5 posizioni
    // Esempio: canale 3 (binario 011) diventa 01100000
    byte shiftedChannel = channel << 5;

    // 3. Uniamo il nuovo canale alla variabile corrente con l'operatore OR (|)
    current_sr_bits |= shiftedChannel;

    // 4. Inviamo allo Shift Register
    updateShiftRegister();
    ESP_LOGD(TAG, "Mux analogico settato sul canale %d",channel);
}


//setup e reset uscite a 1 (relè spento)
void initShiftRegister() {
   pinMode(DATA_PIN, OUTPUT);
   pinMode(LATCH_PIN, OUTPUT);
   pinMode(CLOCK_PIN, OUTPUT);

   current_sr_bits = 0b11111111;
   updateShiftRegister();
   ESP_LOGI(TAG, "Shift register setup: tutto DISATTIVATO (alto)");
}
