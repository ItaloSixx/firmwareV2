#include "utils.h"
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdio.h>
#include <lvgl.h>

static const char *TAG = "UTILS";

/**
 * @brief Converte temperatura de Celsius para Fahrenheit
 */
float utils_celsius_to_fahrenheit(float celsius)
{
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

/**
 * @brief Converte temperatura de Fahrenheit para Celsius
 */
float utils_fahrenheit_to_celsius(float fahrenheit)
{
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

/**
 * @brief Formata uma string com informações de sistema
 */
int utils_format_system_info(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        ESP_LOGE(TAG, "Buffer inválido");
        return -1;
    }
    
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t min_free_heap = esp_get_minimum_free_heap_size();
    
    return lv_snprintf(buffer, buffer_size,
                   "Heap livre: %lu bytes | Heap mín: %lu bytes",
                   free_heap, min_free_heap);
}

/**
 * @brief Obtém timestamp atual em formato string
 */
bool utils_get_timestamp_string(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < 16) {
        ESP_LOGE(TAG, "Buffer muito pequeno para timestamp");
        return false;
    }
    
    uint32_t timestamp = esp_log_timestamp();
    
    // Converte timestamp para horas, minutos, segundos, milissegundos
    uint32_t hours = timestamp / (1000 * 60 * 60);
    uint32_t minutes = (timestamp % (1000 * 60 * 60)) / (1000 * 60);
    uint32_t seconds = (timestamp % (1000 * 60)) / 1000;
    uint32_t milliseconds = timestamp % 1000;
    
    int written = lv_snprintf(buffer, buffer_size, "%02lu:%02lu:%02lu.%03lu",
                          hours, minutes, seconds, milliseconds);
    
    return written > 0 && written < buffer_size;
}

/**
 * @brief Mapeia um valor de um range para outro
 */
float utils_map_value(float value, float from_min, float from_max, float to_min, float to_max)
{
    // Verifica se o range de origem é válido
    if (from_max - from_min == 0) {
        ESP_LOGE(TAG, "Range de origem inválido (min == max)");
        return to_min;
    }
    
    // Calcula a proporção
    float proportion = (value - from_min) / (from_max - from_min);
    
    // Mapeia para o novo range
    return to_min + proportion * (to_max - to_min);
}

/**
 * @brief Limita um valor dentro de um range específico
 */
float utils_clamp(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief Calcula a média de um array de valores
 */
float utils_calculate_average(const float *values, size_t count)
{
    if (values == NULL || count == 0) {
        ESP_LOGE(TAG, "Array inválido ou vazio");
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < count; i++) {
        sum += values[i];
    }
    
    return sum / count;
}
