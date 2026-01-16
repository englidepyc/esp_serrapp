/*Contains deep sleep and other types of setup*/
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "pins.h"

#define uS_TO_S_FACTOR 1000000ULL

static const char* TAG = "DEEP_SLEEP.H";

//prototypes
void print_wakeup_reason();
void deep_sleep_setup(int secondi);

esp_sleep_wakeup_cause_t wakeup_reason;

void print_wakeup_reason(){

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : ESP_LOGE(TAG, "!!!FIRE DETECTED (Normal)!!! Wakeup caused by external signal using RTC_CNTL (Ext1)"); break;
    case ESP_SLEEP_WAKEUP_TIMER : ESP_LOGI(TAG, "Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : ESP_LOGI(TAG, "Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : ESP_LOGI(TAG, "Wakeup caused by ULP program"); break;
    default : ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d", wakeup_reason); break;
  }
}

void deep_sleep_setup(int secondi){
    
    print_wakeup_reason();

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
};