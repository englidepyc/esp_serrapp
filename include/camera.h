#pragma once
#include <Arduino.h>

// Questo header NON include esp_camera.h per evitare conflitti con Adafruit_Sensor.h
// I tipi interni di esp_camera sono nascosti usando void* e int

// Costanti per frame size (valori corrispondenti a framesize_t di esp_camera.h)
#define CAM_FRAMESIZE_VGA    7      // 640x480
#define CAM_FRAMESIZE_SVGA   9      // 800x600
#define CAM_FRAMESIZE_XGA    10     // 1024x768
#define CAM_FRAMESIZE_SXGA   11     // 1280x1024
#define CAM_FRAMESIZE_UXGA   12     // 1600x1200

// Costanti per pixel format (valori corrispondenti a pixformat_t)
#define CAM_PIXFORMAT_JPEG   4

/**
 * @brief Struttura che rappresenta un frame catturato dalla camera
 * Wrapper attorno a camera_fb_t per un'interfaccia più pulita
 */
struct CameraFrame {
    uint8_t *buffer;      // Puntatore al buffer JPEG
    size_t length;        // Dimensione in bytes
    uint16_t width;       // Larghezza immagine
    uint16_t height;      // Altezza immagine
    int format;           // Formato pixel (usa CAM_PIXFORMAT_*)
    void *_internal;      // Puntatore interno opaco (non usare direttamente)
};

/**
 * @brief Parametri di configurazione per la camera
 */
struct CameraConfig {
    int frame_size;           // Risoluzione (usa CAM_FRAMESIZE_*)
    uint8_t jpeg_quality;     // Qualità JPEG (0-63, più basso = migliore)
    uint8_t fb_count;         // Numero di frame buffer (1 o 2)
};

/**
 * @brief Inizializza il sistema camera con configurazione ottimizzata per OV5640
 * 
 * @return true se inizializzazione riuscita, false altrimenti
 * 
 * @note Questa funzione:
 *       - Configura tutti i pin hardware
 *       - Inizializza il driver camera
 *       - Configura i parametri del sensore
 *       - Effettua warm-up del sensore
 *       - Crea mutex per thread-safety
 */
bool camera_init();

/**
 * @brief Inizializza la camera con configurazione personalizzata
 * 
 * @param config Parametri di configurazione
 * @return true se inizializzazione riuscita, false altrimenti
 */
bool camera_init_custom(const CameraConfig &config);

/**
 * @brief Cattura un frame dalla camera
 * 
 * @param frame Puntatore alla struttura CameraFrame da riempire
 * @param timeout_ms Timeout in millisecondi (0 = usa default)
 * @return true se cattura riuscita, false altrimenti
 * 
 * @note Il frame DEVE essere rilasciato con camera_release_frame() dopo l'uso
 * @note Questa funzione è thread-safe
 * 
 * @example
 * CameraFrame frame;
 * if (camera_capture_frame(&frame)) {
 *     // Usa frame.buffer e frame.length
 *     camera_release_frame(&frame);
 * }
 */
bool camera_capture_frame(CameraFrame *frame, uint32_t timeout_ms = 0);

/**
 * @brief Rilascia un frame precedentemente catturato
 * 
 * @param frame Puntatore al frame da rilasciare
 * 
 * @note DEVE essere chiamata per ogni frame catturato per evitare memory leak
 * @note Questa funzione è thread-safe
 */
void camera_release_frame(CameraFrame *frame);

/**
 * @brief Verifica se la camera è inizializzata e pronta
 * 
 * @return true se la camera è pronta, false altrimenti
 */
bool camera_is_ready();

/**
 * @brief Ottiene informazioni sulla configurazione corrente
 * 
 * @param config Puntatore alla struttura da riempire con la configurazione
 * @return true se operazione riuscita, false altrimenti
 */
bool camera_get_config(CameraConfig *config);

/**
 * @brief Deinicializza la camera e libera le risorse
 * 
 * @note Dopo questa chiamata, camera_init() deve essere chiamata di nuovo
 */
void camera_deinit();

/**
 * @brief Ottiene il sensore per configurazioni avanzate
 * 
 * @return Puntatore al sensore (void* per evitare conflitti), NULL se camera non inizializzata
 * 
 * @warning Usare con cautela - modifica diretta del sensore
 * @note Cast a sensor_t* nel file .cpp che include esp_camera.h
 */
void* camera_get_sensor();