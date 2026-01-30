constexpr static const char* MOCK_CAMERA_TAG = "MOCK/CAMERA";
#include "camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstring>

// Nota: NON include esp_camera.h per evitare conflitti

// ========== Mock Configuration ==========
#define MOCK_FRAME_WIDTH   800
#define MOCK_FRAME_HEIGHT  600
#define MOCK_FRAME_SIZE    (MOCK_FRAME_WIDTH * MOCK_FRAME_HEIGHT / 10)  // ~48KB simulato per JPEG

// ========== Global State ==========
static bool g_mock_initialized = false;
static SemaphoreHandle_t g_mock_mutex = NULL;
static CameraConfig g_mock_config;
static uint32_t g_capture_count = 0;  // Contatore per simulare frame diversi

// Simula un frame JPEG (header JPEG minimo + dati fittizi)
static uint8_t g_mock_jpeg_buffer[MOCK_FRAME_SIZE];

// ========== Internal Helper Functions ==========

/**
 * @brief Genera dati JPEG fittizi per simulare una foto reale
 */
static void generate_mock_frame() {
    // Header JPEG valido (SOI marker)
    g_mock_jpeg_buffer[0] = 0xFF;
    g_mock_jpeg_buffer[1] = 0xD8;
    
    // Riempi con pattern riconoscibile (cambia ad ogni cattura)
    for (size_t i = 2; i < MOCK_FRAME_SIZE - 2; i++) {
        g_mock_jpeg_buffer[i] = (uint8_t)((i + g_capture_count) % 256);
    }
    
    // EOI marker JPEG
    g_mock_jpeg_buffer[MOCK_FRAME_SIZE - 2] = 0xFF;
    g_mock_jpeg_buffer[MOCK_FRAME_SIZE - 1] = 0xD9;
    
    ESP_LOGD(MOCK_CAMERA_TAG, "Frame mock generato #%lu", g_capture_count);
}

// ========== Public API Implementation (MOCK) ==========

bool camera_init() {
    CameraConfig default_config;
    default_config.frame_size = CAM_FRAMESIZE_SVGA;
    default_config.jpeg_quality = 12;
    default_config.fb_count = 1;
    
    return camera_init_custom(default_config);
}

bool camera_init_custom(const CameraConfig &config) {
    if (g_mock_initialized) {
        ESP_LOGW(MOCK_CAMERA_TAG, "Mock camera già inizializzata");
        return true;
    }
    
    ESP_LOGI(MOCK_CAMERA_TAG, "Inizializzazione MOCK camera (nessun hardware)");
    
    // Crea mutex per thread-safety
    if (g_mock_mutex == NULL) {
        g_mock_mutex = xSemaphoreCreateMutex();
        if (g_mock_mutex == NULL) {
            ESP_LOGE(MOCK_CAMERA_TAG, "Impossibile creare mutex");
            return false;
        }
    }
    
    // Simula un piccolo delay di inizializzazione
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Salva configurazione
    g_mock_config = config;
    g_capture_count = 0;
    
    // Genera primo frame mock
    generate_mock_frame();
    
    g_mock_initialized = true;
    
    ESP_LOGI(MOCK_CAMERA_TAG, "✓ Mock camera pronta [simulato: %dx%d, Q=%d, FB=%d]",
             MOCK_FRAME_WIDTH, MOCK_FRAME_HEIGHT,
             config.jpeg_quality, config.fb_count);
    ESP_LOGI(MOCK_CAMERA_TAG, "⚠️  NESSUN PIN HARDWARE UTILIZZATO - SOLO SIMULAZIONE");
    
    return true;
}

bool camera_capture_frame(CameraFrame *frame, uint32_t timeout_ms) {
    if (!g_mock_initialized || frame == NULL) {
        ESP_LOGE(MOCK_CAMERA_TAG, "Mock camera non inizializzata o frame NULL");
        return false;
    }
    
    // Timeout management (simulato)
    uint32_t timeout = (timeout_ms == 0) ? 5000 : timeout_ms;
    
    // Lock mutex per thread-safety
    if (xSemaphoreTake(g_mock_mutex, pdMS_TO_TICKS(timeout)) != pdTRUE) {
        ESP_LOGE(MOCK_CAMERA_TAG, "Timeout acquisizione mutex");
        return false;
    }
    
    // Simula tempo di cattura (10-50ms realistici)
    uint32_t capture_delay = 10 + (esp_random() % 40);
    vTaskDelay(pdMS_TO_TICKS(capture_delay));
    
    // Incrementa contatore e rigenera frame
    g_capture_count++;
    generate_mock_frame();
    
    // Popola struttura CameraFrame con dati mock
    frame->buffer = g_mock_jpeg_buffer;
    frame->length = MOCK_FRAME_SIZE;
    frame->width = MOCK_FRAME_WIDTH;
    frame->height = MOCK_FRAME_HEIGHT;
    frame->format = CAM_PIXFORMAT_JPEG;
    frame->_internal = (void*)0xDEADBEEF;  // Puntatore fittizio (NON dereferenziare!)
    
    xSemaphoreGive(g_mock_mutex);
    
    ESP_LOGD(MOCK_CAMERA_TAG, "✓ Mock cattura #%lu: %d bytes, %dx%d (simulato in %lums)",
             g_capture_count, frame->length, frame->width, frame->height, capture_delay);
    
    return true;
}

void camera_release_frame(CameraFrame *frame) {
    if (frame == NULL) {
        ESP_LOGW(MOCK_CAMERA_TAG, "Tentativo di rilasciare frame NULL");
        return;
    }
    
    // In modalità mock non c'è nulla da rilasciare realmente,
    // ma manteniamo il pattern per compatibilità API
    
    // Pulisci struttura
    frame->buffer = NULL;
    frame->length = 0;
    frame->width = 0;
    frame->height = 0;
    frame->_internal = NULL;
    
    ESP_LOGD(MOCK_CAMERA_TAG, "Mock frame rilasciato (no-op)");
}

bool camera_is_ready() {
    return g_mock_initialized;
}

bool camera_get_config(CameraConfig *config) {
    if (!g_mock_initialized || config == NULL) {
        return false;
    }
    
    *config = g_mock_config;
    return true;
}

void camera_deinit() {
    if (!g_mock_initialized) {
        return;
    }
    
    ESP_LOGI(MOCK_CAMERA_TAG, "Deinizializzazione mock camera...");
    
    if (g_mock_mutex) {
        xSemaphoreTake(g_mock_mutex, portMAX_DELAY);
    }
    
    // In mock mode non c'è hardware da de-inizializzare
    // ma manteniamo il cleanup delle risorse software
    
    if (g_mock_mutex) {
        xSemaphoreGive(g_mock_mutex);
        vSemaphoreDelete(g_mock_mutex);
        g_mock_mutex = NULL;
    }
    
    g_mock_initialized = false;
    g_capture_count = 0;
    
    ESP_LOGI(MOCK_CAMERA_TAG, "Mock camera deinizializzata");
}

void* camera_get_sensor() {
    if (!g_mock_initialized) {
        ESP_LOGW(MOCK_CAMERA_TAG, "Mock camera non inizializzata");
        return NULL;
    }
    
    ESP_LOGW(MOCK_CAMERA_TAG, "⚠️  camera_get_sensor() in mock mode restituisce NULL");
    return NULL;  // Non c'è sensore reale in modalità mock
}
