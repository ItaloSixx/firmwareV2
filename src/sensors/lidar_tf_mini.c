/**
 * @file lidar_tf_mini.c
 * @brief Implementação do driver para LiDAR TF Mini Plus
 * @author ItaloSixx
 * @date 2025
 */

#include "lidar_tf_mini.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "LIDAR_TF";

// Configurações do protocolo TF Mini Plus
#define TF_MINI_FRAME_SIZE      9
#define TF_MINI_HEADER_1        0x59
#define TF_MINI_HEADER_2        0x59
#define TF_MINI_BUFFER_SIZE     256
#define TF_MINI_MAX_DISTANCE    1200  // cm
#define TF_MINI_MIN_DISTANCE    10    // cm

// Comandos do TF Mini Plus
#define CMD_GET_VERSION         {0x5A, 0x04, 0x14, 0x00}
#define CMD_RESET               {0x5A, 0x04, 0x02, 0x00}
#define CMD_SET_FREQ_1HZ        {0x5A, 0x06, 0x03, 0x01, 0x00}
#define CMD_SET_FREQ_10HZ       {0x5A, 0x06, 0x03, 0x0A, 0x00}
#define CMD_SET_FREQ_100HZ      {0x5A, 0x06, 0x03, 0x64, 0x00}

// Variáveis globais
static bool g_lidar_initialized = false;
static int g_uart_num = -1;
static lidar_data_t g_last_reading = {0};
static TaskHandle_t g_lidar_task = NULL;
static bool g_task_running = false;

// Função para calcular checksum
static uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return (uint8_t)(checksum & 0xFF);
}

// Função para validar frame TF Mini Plus
static bool validate_frame(const uint8_t *frame) {
    if (frame[0] != TF_MINI_HEADER_1 || frame[1] != TF_MINI_HEADER_2) {
        return false;
    }
    
    uint8_t checksum = calculate_checksum(frame, TF_MINI_FRAME_SIZE - 1);
    return (checksum == frame[TF_MINI_FRAME_SIZE - 1]);
}

// Função para parsear dados do frame
static void parse_frame(const uint8_t *frame, lidar_data_t *data) {
    data->distance = (frame[3] << 8) | frame[2];      // Distância em cm
    data->strength = (frame[5] << 8) | frame[4];      // Força do sinal
    data->temperature = (frame[7] << 8) | frame[6];   // Temperatura
    data->valid = (data->distance >= TF_MINI_MIN_DISTANCE && 
                   data->distance <= TF_MINI_MAX_DISTANCE);
    data->timestamp = esp_timer_get_time() / 1000;    // ms
}

// Task para leitura contínua do LiDAR
static void lidar_read_task(void *pvParameters) {
    uint8_t buffer[TF_MINI_BUFFER_SIZE];
    uint8_t frame[TF_MINI_FRAME_SIZE];
    size_t frame_pos = 0;
    
    ESP_LOGI(TAG, "LiDAR read task started");
    
    while (g_task_running) {
        size_t length = 0;
        
        // Ler dados disponíveis
        esp_err_t ret = uart_get_buffered_data_len(g_uart_num, &length);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "UART get buffered data length failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        if (length == 0) {
            // Log periódico para debug (a cada 5 segundos)
            static uint32_t no_data_count = 0;
            no_data_count++;
            if (no_data_count % 500 == 0) {  // 500 * 10ms = 5s
                ESP_LOGW(TAG, "No UART data available for 5 seconds (count: %lu)", no_data_count);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        // Limitar leitura ao buffer
        if (length > TF_MINI_BUFFER_SIZE) {
            length = TF_MINI_BUFFER_SIZE;
        }
        
        int bytes_read = uart_read_bytes(g_uart_num, buffer, length, pdMS_TO_TICKS(100));
        if (bytes_read <= 0) {
            ESP_LOGD(TAG, "UART read returned %d bytes", bytes_read);
            continue;
        }
        
        ESP_LOGD(TAG, "Received %d bytes from UART", bytes_read);
        
        // Processar bytes recebidos
        for (int i = 0; i < bytes_read; i++) {
            frame[frame_pos] = buffer[i];
            
            // Verificar início do frame
            if (frame_pos == 0 && buffer[i] != TF_MINI_HEADER_1) {
                continue;
            }
            if (frame_pos == 1 && buffer[i] != TF_MINI_HEADER_2) {
                frame_pos = 0;
                continue;
            }
            
            frame_pos++;
            
            // Frame completo recebido
            if (frame_pos == TF_MINI_FRAME_SIZE) {
                if (validate_frame(frame)) {
                    parse_frame(frame, &g_last_reading);
                    ESP_LOGD(TAG, "Distance: %d cm, Strength: %d, Temp: %d", 
                            g_last_reading.distance, 
                            g_last_reading.strength,
                            g_last_reading.temperature);
                } else {
                    ESP_LOGW(TAG, "Invalid frame checksum");
                }
                frame_pos = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz máximo
    }
    
    ESP_LOGI(TAG, "LiDAR read task stopped");
    g_lidar_task = NULL;
    vTaskDelete(NULL);
}

// Implementação das funções públicas
esp_err_t lidar_init(const lidar_config_t *config) {
    if (g_lidar_initialized) {
        ESP_LOGW(TAG, "LiDAR already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing LiDAR TF Mini Plus...");
    ESP_LOGI(TAG, "UART: %d, TX: GPIO%d, RX: GPIO%d, Baud: %d", 
             config->uart_num, config->tx_pin, config->rx_pin, config->baud_rate);
    
    g_uart_num = config->uart_num;
    
    // Configurar UART
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t ret = uart_driver_install(g_uart_num, TF_MINI_BUFFER_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = uart_param_config(g_uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
        uart_driver_delete(g_uart_num);
        return ret;
    }
    
    ret = uart_set_pin(g_uart_num, config->tx_pin, config->rx_pin, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        uart_driver_delete(g_uart_num);
        return ret;
    }
    
    // Limpar buffer
    uart_flush(g_uart_num);
    
    // Teste básico de UART - enviar comando de versão
    ESP_LOGI(TAG, "Testing UART communication...");
    uint8_t test_cmd[] = {0x5A, 0x04, 0x14, 0x00};
    test_cmd[3] = calculate_checksum(test_cmd, 3);
    int bytes_sent = uart_write_bytes(g_uart_num, test_cmd, sizeof(test_cmd));
    ESP_LOGI(TAG, "Sent %d bytes to LiDAR", bytes_sent);
    
    // Aguardar um pouco para possível resposta
    vTaskDelay(pdMS_TO_TICKS(100));
    size_t test_length = 0;
    uart_get_buffered_data_len(g_uart_num, &test_length);
    ESP_LOGI(TAG, "Buffered data after test: %d bytes", test_length);
    
    // Iniciar task de leitura
    g_task_running = true;
    BaseType_t task_ret = xTaskCreate(lidar_read_task, "lidar_read", 
                                      4096, NULL, 5, &g_lidar_task);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LiDAR read task");
        uart_driver_delete(g_uart_num);
        g_task_running = false;
        return ESP_ERR_NO_MEM;
    }
    
    g_lidar_initialized = true;
    ESP_LOGI(TAG, "LiDAR TF Mini Plus initialized successfully");
    
    // Aguardar primeira leitura
    vTaskDelay(pdMS_TO_TICKS(500));
    
    return ESP_OK;
}

void lidar_deinit(void) {
    if (!g_lidar_initialized) {
        return;
    }
    
    // Parar task
    g_task_running = false;
    if (g_lidar_task) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Aguardar task parar
    }
    
    // Limpar UART
    uart_driver_delete(g_uart_num);
    
    g_lidar_initialized = false;
    g_uart_num = -1;
    
    ESP_LOGI(TAG, "LiDAR deinitialized");
}

lidar_error_t lidar_read_blocking(lidar_data_t *data) {
    if (!g_lidar_initialized || !data) {
        return LIDAR_ERROR_INIT;
    }
    
    // Aguardar nova leitura (timeout de 1 segundo)
    uint32_t start_time = esp_timer_get_time() / 1000;
    uint32_t last_timestamp = g_last_reading.timestamp;
    
    while ((esp_timer_get_time() / 1000 - start_time) < 1000) {
        if (g_last_reading.timestamp > last_timestamp && g_last_reading.valid) {
            *data = g_last_reading;
            return LIDAR_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    return LIDAR_ERROR_TIMEOUT;
}

lidar_error_t lidar_read_async(lidar_data_t *data) {
    if (!g_lidar_initialized || !data) {
        return LIDAR_ERROR_INIT;
    }
    
    if (g_last_reading.valid) {
        *data = g_last_reading;
        return LIDAR_OK;
    }
    
    return LIDAR_ERROR_INVALID_DATA;
}

bool lidar_data_available(void) {
    return g_lidar_initialized && g_last_reading.valid;
}

bool lidar_get_last_reading(lidar_data_t *data) {
    if (!g_lidar_initialized || !data) {
        return false;
    }
    
    *data = g_last_reading;
    return g_last_reading.valid;
}

lidar_error_t lidar_set_frequency(uint16_t freq_hz) {
    if (!g_lidar_initialized) {
        return LIDAR_ERROR_INIT;
    }
    
    uint8_t cmd[5] = {0x5A, 0x06, 0x03, 0x00, 0x00};
    cmd[3] = (uint8_t)(freq_hz & 0xFF);
    cmd[4] = (uint8_t)((freq_hz >> 8) & 0xFF);
    
    // Calcular checksum
    uint8_t checksum = 0;
    for (int i = 0; i < 4; i++) {
        checksum += cmd[i];
    }
    cmd[4] = checksum;
    
    int bytes_written = uart_write_bytes(g_uart_num, cmd, sizeof(cmd));
    if (bytes_written != sizeof(cmd)) {
        return LIDAR_ERROR_COMMUNICATION;
    }
    
    ESP_LOGI(TAG, "Set frequency to %d Hz", freq_hz);
    return LIDAR_OK;
}

lidar_error_t lidar_reset(void) {
    if (!g_lidar_initialized) {
        return LIDAR_ERROR_INIT;
    }
    
    uint8_t cmd[] = {0x5A, 0x04, 0x02, 0x00};
    cmd[3] = calculate_checksum(cmd, 3);
    
    int bytes_written = uart_write_bytes(g_uart_num, cmd, sizeof(cmd));
    if (bytes_written != sizeof(cmd)) {
        return LIDAR_ERROR_COMMUNICATION;
    }
    
    // Aguardar reset
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "LiDAR reset");
    return LIDAR_OK;
}

const char* lidar_error_to_string(lidar_error_t error) {
    switch (error) {
        case LIDAR_OK:                  return "OK";
        case LIDAR_ERROR_INIT:          return "Not initialized";
        case LIDAR_ERROR_TIMEOUT:       return "Timeout";
        case LIDAR_ERROR_CHECKSUM:      return "Checksum error";
        case LIDAR_ERROR_INVALID_DATA:  return "Invalid data";
        case LIDAR_ERROR_COMMUNICATION: return "Communication error";
        default:                        return "Unknown error";
    }
}