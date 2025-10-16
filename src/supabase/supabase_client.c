/**
 * @file supabase_client.c
 * @brief Supabase REST API client implementation
 */

#include "supabase_client.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

static const char *TAG = "supabase";

// Supabase configuration
#define SUPABASE_URL        "https://btrhboofdkxdqbqhefen.supabase.co"
#define SUPABASE_KEY        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJ0cmhib29mZGt4ZHFicWhlZmVuIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjA2Mjc4NDAsImV4cCI6MjA3NjIwMzg0MH0.GQE9VJtXxj1ZTrzs2myE7y2KinswMRgrEtfNeZkswLU"
#define SUPABASE_TABLE      "measurements"
#define SUPABASE_ENDPOINT   SUPABASE_URL "/rest/v1/" SUPABASE_TABLE

#define HTTP_TIMEOUT_MS     10000
#define HTTP_BUFFER_SIZE    2048

static bool s_initialized = false;

/**
 * @brief HTTP event handler
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP headers sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "Header: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(TAG, "Response: %.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP disconnected");
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief Task to send measurement to Supabase (runs in background)
 */
static void supabase_send_task(void *param)
{
    ESP_LOGI(TAG, "=== Supabase send task started ===");
    
    supabase_measurement_t *measurement = (supabase_measurement_t *)param;
    
    if (!measurement) {
        ESP_LOGE(TAG, "Invalid measurement pointer");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Creating JSON payload...");
    
    // Create JSON payload
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        free(measurement);
        vTaskDelete(NULL);
        return;
    }

    cJSON_AddStringToObject(root, "timestamp", measurement->timestamp);
    cJSON_AddNumberToObject(root, "horizontal_cm", measurement->horizontal_cm);
    cJSON_AddNumberToObject(root, "top_cm", measurement->top_cm);
    cJSON_AddNumberToObject(root, "base_cm", measurement->base_cm);
    cJSON_AddNumberToObject(root, "height_top_cm", measurement->height_top_cm);
    cJSON_AddNumberToObject(root, "height_base_cm", measurement->height_base_cm);
    cJSON_AddNumberToObject(root, "total_cm", measurement->total_cm);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to create JSON string");
        free(measurement);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "JSON payload created: %s", json_string);

    ESP_LOGI(TAG, "Initializing HTTP client to: %s", SUPABASE_ENDPOINT);
    
    // Configure HTTP client with TLS settings
    esp_http_client_config_t config = {
        .url = SUPABASE_ENDPOINT,
        .event_handler = http_event_handler,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .buffer_size = HTTP_BUFFER_SIZE,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = false,
        .crt_bundle_attach = esp_crt_bundle_attach,  // Use ESP32 certificate bundle
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(json_string);
        free(measurement);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "HTTP client initialized, setting headers...");

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "apikey", SUPABASE_KEY);
    esp_http_client_set_header(client, "Authorization", "Bearer " SUPABASE_KEY);
    esp_http_client_set_header(client, "Prefer", "return=minimal");

    ESP_LOGI(TAG, "Headers set, posting data...");

    // Set POST data
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    ESP_LOGI(TAG, "Performing HTTP POST request...");
    
    // Perform HTTP request
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d", status_code);
        
        if (status_code == 201 || status_code == 200) {
            ESP_LOGI(TAG, "✓ Measurement sent to Supabase successfully!");
        } else {
            ESP_LOGW(TAG, "⚠ Unexpected status code: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "✗ HTTP POST request failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Cleaning up...");

    // Cleanup
    esp_http_client_cleanup(client);
    free(json_string);
    free(measurement);
    
    ESP_LOGI(TAG, "=== Supabase send task finished ===");
    vTaskDelete(NULL);
}

esp_err_t supabase_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Supabase client already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing Supabase client");
    ESP_LOGI(TAG, "Endpoint: %s", SUPABASE_ENDPOINT);
    
    s_initialized = true;
    return ESP_OK;
}

esp_err_t supabase_send_measurement(const supabase_measurement_t *measurement)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Supabase client not initialized");
        return ESP_FAIL;
    }

    if (!measurement) {
        ESP_LOGE(TAG, "Invalid measurement pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate memory for measurement copy (will be freed in task)
    supabase_measurement_t *measurement_copy = malloc(sizeof(supabase_measurement_t));
    if (!measurement_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for measurement");
        return ESP_ERR_NO_MEM;
    }

    memcpy(measurement_copy, measurement, sizeof(supabase_measurement_t));

    // Create task to send measurement in background
    BaseType_t result = xTaskCreate(
        supabase_send_task,
        "supabase_send",
        4096,
        measurement_copy,
        5,
        NULL
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Supabase send task");
        free(measurement_copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool supabase_is_ready(void)
{
    return s_initialized;
}
