// Not used anymore as of 11.11.26
#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "i2c_io_expander.h"
#include "i2c_mux_and_sensor_reads.h"


constexpr static const char* IO_EXP_TAG = "IO_EXPANDER";
static const char* deviceNames[] = {"PUMP", "EXT_FAN", "AMUX0", "AMUX1", "AMUX2"};

// Variabile "ombra" per tenere traccia dello stato dei bit
// 0xFF significa tutti i bit a 1 (HIGH). 
// Poiché i relè sono Active LOW, all'avvio è tutto SPENTO.
static byte _port_state = 0xFF; 

namespace io_expander {

// Funzione privata per inviare fisicamente i dati via I2C
void _writePCF(byte data) {
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(data);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        ESP_LOGE(IO_EXP_TAG, "Errore I2C durante la scrittura! Codice: %d", error);
    }
}

void updateIOExpander() {
    i2c_mux_select_bus(PCF_MUX_CHANNEL);
    _writePCF(_port_state);
}

void setAllOutputs(bool state) {
    // Se state è TRUE (HIGH), scriviamo 0xFF (tutti spenti se active low)
    // Se state è FALSE (LOW), scriviamo 0x00 (tutti accesi)
    // Nota: Ho mantenuto la logica bitwise del tuo codice originale:
    // state ? 1 : 0. 
    _port_state = (state ? 0xFF : 0x00);
    updateIOExpander();
}

void setDevice(Device dev, bool state) {
    // Aggiornamento della variabile ombra
    // Se state è HIGH (1), settiamo il bit (Relè OFF)
    // Se state è LOW (0), puliamo il bit (Relè ON)
    if (state) {
        bitSet(_port_state, dev);
    } else {
        bitClear(_port_state, dev);
    }

    updateIOExpander();

    // Logica di logging (Invertita perché RELAY_ON è LOW)
    if (state == RELAY_ON) {
        // Controllo bounds per evitare crash sull'array dei nomi
        if(dev <= 1) ESP_LOGI(IO_EXP_TAG, "%s attivato!", deviceNames[dev]);
    } else {
        if(dev <= 1) ESP_LOGI(IO_EXP_TAG, "%s disattivato", deviceNames[dev]);
    }
}

void setAmuxChannel(byte channel) {
   if (channel > 7){
      ESP_LOGW(IO_EXP_TAG, "Canale mux analogico non valido: %d", channel);
      return;
   } 
    
   // 1. Puliamo i bit dedicati all'AMUX (P2, P3, P4)
   // Maschera inversa: Vogliamo preservare tutto tranne bit 2,3,4.
   // 0b00011100 è 0x1C. L'inverso (~) protegge gli altri bit.
   _port_state &= ~0b00011100; 

   // 2. Spostiamo il canale nella posizione corretta
   // I pin AMUX partono da P2, quindi shiftiamo a sinistra di 2.
   // Es: Canale 3 (011) diventa (00001100) -> P3 e P2 attivi.
   byte shiftedChannel = channel << 2;

   // 3. Uniamo alla variabile ombra
   _port_state |= shiftedChannel;

   // 4. Inviamo I2C
   updateIOExpander();
   ESP_LOGD(IO_EXP_TAG, "Mux analogico settato sul canale %d (Raw: 0x%02X)", channel, _port_state);
}

// Reset iniziale: Tutto HIGH (Relè Spenti)
void resetIOExpander() {
    _port_state = 0xFF;
    updateIOExpander();
    
    ESP_LOGI(IO_EXP_TAG, "IO Expander (PCF8574) inizializzato a 0xFF (Tutto OFF)");
}

}