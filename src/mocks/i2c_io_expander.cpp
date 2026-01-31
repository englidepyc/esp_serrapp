#include <Arduino.h>
#include "pins.h"
#include "i2c_io_expander.h"


constexpr static const char* IO_EXP_TAG = "IO_EXPANDER_MOCK";

void updateIOExpander() {
    // Mock: non fa nulla
}

void setAllOutputs(bool state) {
    // Mock: non fa nulla
    ESP_LOGD(IO_EXP_TAG, "setAllOutputs(%d) - MOCK", state);
}

void setDevice(Device dev, bool state) {
    // Mock: non fa nulla
    ESP_LOGD(IO_EXP_TAG, "setDevice(%d, %d) - MOCK", dev, state);
}

void setAmuxChannel(byte channel) {
    // Mock: non fa nulla
    ESP_LOGD(IO_EXP_TAG, "setAmuxChannel(%d) - MOCK", channel);
}

void initIOExpander() {
    // Mock: non fa nulla
    ESP_LOGI(IO_EXP_TAG, "IO Expander inizializzato (MOCK - nessuna operazione reale)");
}

void resetIOExpander(){
    ESP_LOGI(IO_EXP_TAG, "IO Expander resettato (MOCK - nessuna operazione reale)");
}
