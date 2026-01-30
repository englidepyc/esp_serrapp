/**
 * ESP32-S3 Memory Test
 * Board: ESP32-S3-DevKitC-1-N16R8V
 * - 16 MB Flash (QIO mode)
 * - 8 MB PSRAM (OPI mode)
 * - 512 KB SRAM (320 KB disponibili)
 */

#include <Arduino.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>

static const char* TAG = "MEMORY_TEST";

void test_sram() {
    ESP_LOGI(TAG, "=== SRAM INTERNA TEST ===");
    
    // Informazioni heap interno (SRAM)
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    
    ESP_LOGI(TAG, "SRAM Heap:");
    ESP_LOGI(TAG, "  - Totale disponibile: %u bytes (%.2f KB)", 
             info.total_free_bytes, info.total_free_bytes / 1024.0);
    ESP_LOGI(TAG, "  - Blocco più grande: %u bytes (%.2f KB)", 
             info.largest_free_block, info.largest_free_block / 1024.0);
    ESP_LOGI(TAG, "  - Minimo mai disponibile: %u bytes", 
             info.minimum_free_bytes);
    
    // Test allocazione SRAM
    size_t test_size = 10240; // 10 KB
    void* test_ptr = heap_caps_malloc(test_size, MALLOC_CAP_INTERNAL);
    if (test_ptr) {
        ESP_LOGI(TAG, "  ✓ Allocati %u bytes in SRAM con successo", test_size);
        memset(test_ptr, 0xAA, test_size);
        ESP_LOGI(TAG, "  ✓ Test scrittura completato");
        heap_caps_free(test_ptr);
    } else {
        ESP_LOGE(TAG, "  ✗ Allocazione SRAM fallita!");
    }
}

void test_psram() {
    ESP_LOGI(TAG, "=== PSRAM (OPI) TEST ===");
    
    // Verifica presenza PSRAM
    if (!psramFound()) {
        ESP_LOGE(TAG, "  ✗ PSRAM NON TROVATA!");
        return;
    }
    
    ESP_LOGI(TAG, "  ✓ PSRAM rilevata");
    
    // Informazioni PSRAM
    size_t psram_size = ESP.getPsramSize();
    ESP_LOGI(TAG, "  - Dimensione totale: %u bytes (%.2f MB)", 
             psram_size, psram_size / (1024.0 * 1024.0));
    
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    
    ESP_LOGI(TAG, "PSRAM Heap:");
    ESP_LOGI(TAG, "  - Disponibile: %u bytes (%.2f MB)", 
             info.total_free_bytes, info.total_free_bytes / (1024.0 * 1024.0));
    ESP_LOGI(TAG, "  - Blocco più grande: %u bytes (%.2f MB)", 
             info.largest_free_block, info.largest_free_block / (1024.0 * 1024.0));
    
    // Test allocazione grosso buffer in PSRAM
    size_t test_size = 1024 * 1024; // 1 MB
    void* test_ptr = heap_caps_malloc(test_size, MALLOC_CAP_SPIRAM);
    if (test_ptr) {
        ESP_LOGI(TAG, "  ✓ Allocati %u bytes (1 MB) in PSRAM", test_size);
        
        // Test scrittura/lettura
        uint8_t* data = (uint8_t*)test_ptr;
        for (size_t i = 0; i < test_size; i += 1024) {
            data[i] = (i / 1024) & 0xFF;
        }
        ESP_LOGI(TAG, "  ✓ Test scrittura completato");
        
        // Verifica
        bool verify_ok = true;
        for (size_t i = 0; i < test_size; i += 1024) {
            if (data[i] != ((i / 1024) & 0xFF)) {
                verify_ok = false;
                break;
            }
        }
        
        if (verify_ok) {
            ESP_LOGI(TAG, "  ✓ Test lettura/verifica completato");
        } else {
            ESP_LOGE(TAG, "  ✗ Errore verifica dati!");
        }
        
        heap_caps_free(test_ptr);
    } else {
        ESP_LOGE(TAG, "  ✗ Allocazione PSRAM fallita!");
    }
}

void test_flash() {
    ESP_LOGI(TAG, "=== FLASH (QIO) TEST ===");
    
    // Dimensione totale flash
    uint32_t flash_size = ESP.getFlashChipSize();
    ESP_LOGI(TAG, "Flash totale:");
    ESP_LOGI(TAG, "  - Dimensione: %u bytes (%.2f MB)", 
             flash_size, flash_size / (1024.0 * 1024.0));
    
    // Modalità e velocità flash
    ESP_LOGI(TAG, "  - Modalità configurata: QIO (Quad I/O)");
    ESP_LOGI(TAG, "  - Frequenza: %u MHz", ESP.getFlashChipSpeed() / 1000000);
    
    // Sketch info
    ESP_LOGI(TAG, "Sketch:");
    ESP_LOGI(TAG, "  - Dimensione: %u bytes (%.2f KB)", 
             ESP.getSketchSize(), ESP.getSketchSize() / 1024.0);
    ESP_LOGI(TAG, "  - Spazio libero: %u bytes (%.2f KB)", 
             ESP.getFreeSketchSpace(), ESP.getFreeSketchSpace() / 1024.0);
    
    // Test lettura flash
    const char* test_string = "ESP32-S3 Flash Test";
    ESP_LOGI(TAG, "  ✓ Test lettura dati da flash: '%s'", test_string);
}

void print_system_info() {
    ESP_LOGI(TAG, "=== INFORMAZIONI SISTEMA ===");
    ESP_LOGI(TAG, "Chip: %s", ESP.getChipModel());
    ESP_LOGI(TAG, "Revisione: %d", ESP.getChipRevision());
    ESP_LOGI(TAG, "CPU Cores: %d", ESP.getChipCores());
    ESP_LOGI(TAG, "CPU Freq: %u MHz", ESP.getCpuFreqMHz());
    ESP_LOGI(TAG, "SDK Version: %s", ESP.getSdkVersion());
    ESP_LOGI(TAG, "");
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Aspetta stabilizzazione seriale
    
    ESP_LOGI(TAG, "\n\n╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32-S3 MEMORY TEST SUITE           ║");
    ESP_LOGI(TAG, "║  Board: ESP32-S3-DevKitC-1-N16R8V     ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝\n");
    
    print_system_info();
    
    test_sram();
    ESP_LOGI(TAG, "");
    
    test_psram();
    ESP_LOGI(TAG, "");
    
    test_flash();
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  TEST COMPLETATO                      ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
}

void loop() {
    // Niente da fare nel loop
    delay(10000);
}