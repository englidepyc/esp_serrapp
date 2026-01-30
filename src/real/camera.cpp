constexpr static const char* R_CAMERA_TAG = "CAMERA";
#include "camera.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ========== Pin Configuration (ESP32-S3 with OV5640) ==========
#define CAM_PIN_PWDN    -1  // Power down (non usato)
#define CAM_PIN_RESET   -1  // Reset (non usato)
#define CAM_PIN_XCLK    15  // Clock
#define CAM_PIN_SIOD    4   // I2C SDA
#define CAM_PIN_SIOC    5   // I2C SCL
#define CAM_PIN_D7      18  // Y9 (data bit 7)
#define CAM_PIN_D6      17  // Y8
#define CAM_PIN_D5      16  // Y7
#define CAM_PIN_D4      12  // Y6
#define CAM_PIN_D3      10  // Y5
#define CAM_PIN_D2      8   // Y4
#define CAM_PIN_D1      9   // Y3
#define CAM_PIN_D0      11  // Y2 (data bit 0)
#define CAM_PIN_VSYNC   6   // Vertical sync
#define CAM_PIN_HREF    7   // Horizontal reference
#define CAM_PIN_PCLK    13  // Pixel clock

// ========== Default Configuration ==========
// Nota: i nostri enum camera_framesize_t mappano direttamente a framesize_t di esp_camera.h
#define DEFAULT_FRAME_SIZE   FRAMESIZE_SVGA  // 800x600 - ottimale per OV5640
#define DEFAULT_JPEG_QUALITY 12               // Bilanciato
#define DEFAULT_FB_COUNT     1                // Singolo buffer per evitare surriscaldamento
#define XCLK_FREQ_HZ         20000000        // 20 MHz
#define CAPTURE_TIMEOUT_MS   5000            // 5 secondi default

// ========== Global State ==========
static bool g_camera_initialized = false;
static SemaphoreHandle_t g_camera_mutex = NULL;
static CameraConfig g_current_config;

// ========== Internal Helper Functions ==========

/**
 * @brief Configura i parametri del sensore OV5640 per operazione ottimale
 */
static bool configure_ov5640_sensor() {
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        ESP_LOGE(R_CAMERA_TAG, "Impossibile ottenere riferimento sensore");
        return false;
    }
    
    // Log del tipo di sensore rilevato
    ESP_LOGI(R_CAMERA_TAG, "Sensore rilevato: PID=0x%x", s->id.PID);
    
    // Configurazione ottimizzata per OV5640
    s->set_brightness(s, 0);     // -2 a 2
    s->set_contrast(s, 0);       // -2 a 2
    s->set_saturation(s, 0);     // -2 a 2
    s->set_special_effect(s, 0); // No effetti speciali
    s->set_whitebal(s, 1);       // Abilita white balance automatico
    s->set_awb_gain(s, 1);       // Abilita guadagno AWB
    s->set_wb_mode(s, 0);        // Auto white balance mode
    s->set_exposure_ctrl(s, 1);  // Abilita controllo esposizione
    s->set_aec2(s, 1);           // Abilita AEC sensor
    s->set_ae_level(s, 0);       // Livello auto-esposizione normale
    s->set_aec_value(s, 300);    // Valore esposizione
    s->set_gain_ctrl(s, 1);      // Abilita controllo guadagno
    s->set_agc_gain(s, 0);       // Guadagno automatico
    s->set_gainceiling(s, (gainceiling_t)0);  // Limite guadagno
    s->set_bpc(s, 0);            // Black pixel cancel off
    s->set_wpc(s, 1);            // White pixel cancel on
    s->set_raw_gma(s, 1);        // Gamma on
    s->set_lenc(s, 1);           // Lens correction on
    s->set_hmirror(s, 0);        // No horizontal mirror
    s->set_vflip(s, 0);          // No vertical flip
    s->set_dcw(s, 1);            // Downsize enable
    s->set_colorbar(s, 0);       // Disabilita test pattern
    
    ESP_LOGI(R_CAMERA_TAG, "Sensore OV5640 configurato");
    return true;
}

/**
 * @brief Warm-up del sensore scartando i primi frame
 */
static void warmup_sensor() {
    ESP_LOGI(R_CAMERA_TAG, "Avvio warm-up sensore...");
    
    for (int i = 0; i < 2; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            esp_camera_fb_return(fb);
            ESP_LOGD(R_CAMERA_TAG, "Frame warm-up %d/%d completato", i + 1, 2);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            ESP_LOGW(R_CAMERA_TAG, "Frame warm-up %d/%d fallito (normale)", i + 1, 2);
        }
    }
    
    ESP_LOGI(R_CAMERA_TAG, "Warm-up completato, camera pronta");
}

// ========== Public API Implementation ==========

bool camera_init() {
    CameraConfig default_config;
    default_config.frame_size = CAM_FRAMESIZE_SVGA;
    default_config.jpeg_quality = DEFAULT_JPEG_QUALITY;
    default_config.fb_count = DEFAULT_FB_COUNT;
    
    return camera_init_custom(default_config);
}

bool camera_init_custom(const CameraConfig &config) {
    if (g_camera_initialized) {
        ESP_LOGW(R_CAMERA_TAG, "Camera già inizializzata");
        return true;
    }
    
    ESP_LOGI(R_CAMERA_TAG, "Inizializzazione camera OV5640...");
    
    // Crea mutex per thread-safety
    if (g_camera_mutex == NULL) {
        g_camera_mutex = xSemaphoreCreateMutex();
        if (g_camera_mutex == NULL) {
            ESP_LOGE(R_CAMERA_TAG, "Impossibile creare mutex");
            return false;
        }
    }
    
    // Configura struttura camera_config_t
    camera_config_t cam_config;
    cam_config.ledc_channel = LEDC_CHANNEL_0;
    cam_config.ledc_timer = LEDC_TIMER_0;
    cam_config.pin_d0 = CAM_PIN_D0;
    cam_config.pin_d1 = CAM_PIN_D1;
    cam_config.pin_d2 = CAM_PIN_D2;
    cam_config.pin_d3 = CAM_PIN_D3;
    cam_config.pin_d4 = CAM_PIN_D4;
    cam_config.pin_d5 = CAM_PIN_D5;
    cam_config.pin_d6 = CAM_PIN_D6;
    cam_config.pin_d7 = CAM_PIN_D7;
    cam_config.pin_xclk = CAM_PIN_XCLK;
    cam_config.pin_pclk = CAM_PIN_PCLK;
    cam_config.pin_vsync = CAM_PIN_VSYNC;
    cam_config.pin_href = CAM_PIN_HREF;
    cam_config.pin_sccb_sda = CAM_PIN_SIOD;
    cam_config.pin_sccb_scl = CAM_PIN_SIOC;
    cam_config.pin_pwdn = CAM_PIN_PWDN;
    cam_config.pin_reset = CAM_PIN_RESET;
    cam_config.xclk_freq_hz = XCLK_FREQ_HZ;
    cam_config.pixel_format = PIXFORMAT_JPEG;
    
    // Applica configurazione utente
    // Cast da int dell'API pubblica ai tipi enum di esp_camera.h
    cam_config.frame_size = (framesize_t)config.frame_size;
    cam_config.jpeg_quality = config.jpeg_quality;
    cam_config.fb_count = config.fb_count;
    
    // Configurazione PSRAM
    if (psramFound()) {
        cam_config.fb_location = CAMERA_FB_IN_PSRAM;
        cam_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  // Importante per OV5640
        ESP_LOGI(R_CAMERA_TAG, "PSRAM rilevata - buffer in PSRAM");
    } else {
        cam_config.fb_location = CAMERA_FB_IN_DRAM;
        cam_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        ESP_LOGW(R_CAMERA_TAG, "PSRAM non rilevata - buffer in DRAM");
    }
    
    // Inizializza driver camera
    esp_err_t err = esp_camera_init(&cam_config);
    if (err != ESP_OK) {
        ESP_LOGE(R_CAMERA_TAG, "Inizializzazione fallita: 0x%x", err);
        if (g_camera_mutex) {
            vSemaphoreDelete(g_camera_mutex);
            g_camera_mutex = NULL;
        }
        return false;
    }
    
    ESP_LOGI(R_CAMERA_TAG, "Driver camera inizializzato");
    
    // Configura sensore OV5640
    if (!configure_ov5640_sensor()) {
        esp_camera_deinit();
        if (g_camera_mutex) {
            vSemaphoreDelete(g_camera_mutex);
            g_camera_mutex = NULL;
        }
        return false;
    }
    
    // Warm-up del sensore
    warmup_sensor();
    
    // Salva configurazione corrente
    g_current_config = config;
    g_camera_initialized = true;
    
    ESP_LOGI(R_CAMERA_TAG, "✓ Camera pronta [%dx%d, Q=%d, FB=%d]",
             config.frame_size, config.frame_size, 
             config.jpeg_quality, config.fb_count);
    
    return true;
}

bool camera_capture_frame(CameraFrame *frame, uint32_t timeout_ms) {
    if (!g_camera_initialized || frame == NULL) {
        ESP_LOGE(R_CAMERA_TAG, "Camera non inizializzata o frame NULL");
        return false;
    }
    
    // Timeout management
    uint32_t timeout = (timeout_ms == 0) ? CAPTURE_TIMEOUT_MS : timeout_ms;
    
    // Lock mutex per thread-safety
    if (xSemaphoreTake(g_camera_mutex, pdMS_TO_TICKS(timeout)) != pdTRUE) {
        ESP_LOGE(R_CAMERA_TAG, "Timeout acquisizione mutex");
        return false;
    }
    
    // Tentativo di cattura con retry
    camera_fb_t *fb = NULL;
    const int max_retries = 3;
    
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        ESP_LOGD(R_CAMERA_TAG, "Tentativo cattura %d/%d", attempt, max_retries);
        
        fb = esp_camera_fb_get();
        
        if (fb != NULL && fb->len > 0) {
            ESP_LOGD(R_CAMERA_TAG, "✓ Cattura OK: %d bytes, %dx%d", 
                     fb->len, fb->width, fb->height);
            break;
        }
        
        if (fb != NULL) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        
        ESP_LOGW(R_CAMERA_TAG, "✗ Tentativo %d fallito", attempt);
        
        if (attempt < max_retries) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    xSemaphoreGive(g_camera_mutex);
    
    if (fb == NULL || fb->len == 0) {
        ESP_LOGE(R_CAMERA_TAG, "Cattura fallita dopo %d tentativi", max_retries);
        if (fb) {
            esp_camera_fb_return(fb);
        }
        return false;
    }
    
    // Popola struttura CameraFrame
    frame->buffer = fb->buf;
    frame->length = fb->len;
    frame->width = fb->width;
    frame->height = fb->height;
    frame->format = (int)fb->format;
    frame->_internal = (void*)fb;  // Mantieni riferimento interno
    
    return true;
}

void camera_release_frame(CameraFrame *frame) {
    if (frame == NULL || frame->_internal == NULL) {
        ESP_LOGW(R_CAMERA_TAG, "Tentativo di rilasciare frame NULL");
        return;
    }
    
    esp_camera_fb_return((camera_fb_t*)frame->_internal);
    
    // Pulisci struttura
    frame->buffer = NULL;
    frame->length = 0;
    frame->width = 0;
    frame->height = 0;
    frame->_internal = NULL;
    
    ESP_LOGD(R_CAMERA_TAG, "Frame rilasciato");
}

bool camera_is_ready() {
    return g_camera_initialized;
}

bool camera_get_config(CameraConfig *config) {
    if (!g_camera_initialized || config == NULL) {
        return false;
    }
    
    *config = g_current_config;
    return true;
}

void camera_deinit() {
    if (!g_camera_initialized) {
        return;
    }
    
    ESP_LOGI(R_CAMERA_TAG, "Deinizializzazione camera...");
    
    if (g_camera_mutex) {
        xSemaphoreTake(g_camera_mutex, portMAX_DELAY);
    }
    
    esp_camera_deinit();
    
    if (g_camera_mutex) {
        xSemaphoreGive(g_camera_mutex);
        vSemaphoreDelete(g_camera_mutex);
        g_camera_mutex = NULL;
    }
    
    g_camera_initialized = false;
    ESP_LOGI(R_CAMERA_TAG, "Camera deinizializzata");
}

void* camera_get_sensor() {
    if (!g_camera_initialized) {
        ESP_LOGW(R_CAMERA_TAG, "Camera non inizializzata");
        return NULL;
    }
    
    return (void*)esp_camera_sensor_get();
}