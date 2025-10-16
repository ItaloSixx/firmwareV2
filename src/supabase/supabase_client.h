/**
 * @file supabase_client.h
 * @brief Supabase REST API client for ESP32
 * 
 * Provides functions to send measurement data to Supabase database
 */

#ifndef SUPABASE_CLIENT_H
#define SUPABASE_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Measurement data structure matching Supabase table schema
 */
typedef struct {
    char timestamp[32];          // ISO8601 format: YYYY-MM-DDTHH:MM:SSZ
    float horizontal_cm;
    float top_cm;
    float base_cm;
    float height_top_cm;
    float height_base_cm;
    float total_cm;
} supabase_measurement_t;

/**
 * @brief Initialize Supabase client
 * 
 * Must be called before using any other supabase functions.
 * Requires Wi-Fi connection to be established.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t supabase_init(void);

/**
 * @brief Send measurement data to Supabase
 * 
 * Sends a POST request to Supabase REST API to insert measurement data.
 * This function is non-blocking and runs in a separate task.
 * 
 * @param measurement Pointer to measurement data structure
 * @return ESP_OK if request initiated successfully, ESP_FAIL on error
 */
esp_err_t supabase_send_measurement(const supabase_measurement_t *measurement);

/**
 * @brief Check if Supabase client is initialized and ready
 * 
 * @return true if initialized, false otherwise
 */
bool supabase_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif // SUPABASE_CLIENT_H
