/*Contains deep sleep and other types of setup*/
constexpr static const char* DEEP_SLEEP_H_TAG = "DEEP_SLEEP.CPP";
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "pins.h"
#include "deep_sleep.h"

void print_wakeup_reason(esp_sleep_wakeup_cause_t wr){

  switch(wr)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : ESP_LOGI(DEEP_SLEEP_H_TAG, "Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : ESP_LOGE(DEEP_SLEEP_H_TAG, "!!!FIRE DETECTED (Normal)!!! Wakeup caused by external signal using RTC_CNTL (Ext1)"); break;
    case ESP_SLEEP_WAKEUP_TIMER : ESP_LOGI(DEEP_SLEEP_H_TAG, "Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : ESP_LOGI(DEEP_SLEEP_H_TAG, "Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : ESP_LOGI(DEEP_SLEEP_H_TAG, "Wakeup caused by ULP program"); break;
    default : ESP_LOGI(DEEP_SLEEP_H_TAG, "Wakeup was not caused by deep sleep: %d", wr); break;
  }
}



//setup per il risveglio sblocca e setta il pin rtc, timer e restituisce causa risveglio
void deep_sleep_setup(int secondi){

    // the pull down etc is "sticky" so it remains while awake    
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
    esp_sleep_enable_timer_wakeup(secondi * uS_TO_S_FACTOR);
    ESP_LOGI(DEEP_SLEEP_H_TAG, "Deep Sleep setup complete with timer %d seconds!", secondi);
}