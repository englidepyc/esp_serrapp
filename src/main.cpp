#include <Arduino.h>
#include "shift_register.h"
#include "driver/rtc_io.h"

#define WAKE_PIN 2
#define uS_TO_S_FACTOR 1000000ULL

//prototypes
void print_wakeup_reason();

esp_sleep_wakeup_cause_t wakeup_reason;


void setup() {
    Serial.begin(115200);
    print_wakeup_reason();

    
    // 1. Initialize the pin for RTC use
    rtc_gpio_init((gpio_num_t)WAKE_PIN);
    rtc_gpio_set_direction((gpio_num_t)WAKE_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    
    // 2. Enable the internal PULL-DOWN
    rtc_gpio_pulldown_en((gpio_num_t)WAKE_PIN);
    rtc_gpio_pullup_dis((gpio_num_t)WAKE_PIN);
    
    // 3. IMPORTANT: Hold the resistor state during sleep
    rtc_gpio_hold_en((gpio_num_t)WAKE_PIN);

    // 4. Configure Ext1
    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);

    esp_sleep_enable_timer_wakeup(5 * uS_TO_S_FACTOR);

}

void loop() {
    
    Serial.println("Waiting 5 seconds before deep sleep...");
    delay(5000);
    Serial.println("Gonna sleep now\n");
    esp_deep_sleep_start();
    
}

void print_wakeup_reason(){

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}