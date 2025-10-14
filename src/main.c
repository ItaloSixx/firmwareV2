/**
 * @file main.c
 * @brief Aplicacao principal do JC3248W535 com ESP-IDF Framework
 * @author Seu Nome
 * @date 2025
 */

#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <string.h>
#include <nvs_flash.h>
#include <time.h>
#include <driver/uart.h>
#include "config.h"
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"

// Inclusao dos modulos do projeto
#include "interface/ui_os.h"
#include "sensors/sensors.h"
#include "sensors/lidar_tf_mini.h"

// Variaveis globais
static sensor_data_t sensor_data = {0};
static const char *TAG = "MAIN";

/**
 * @brief Inicializa o sistema
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3 Operating System ===");
    ESP_LOGI(TAG, "Inicializando sistema embarcado...");
    
    // Inicializar NVS Flash para persistência de dados
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash inicializado!");
    
    // Configurar timezone para UTC para evitar problemas de conversão
    setenv("TZ", "UTC", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone configurado para UTC");
    
    // Inicializa display (usando codigo ESP-IDF existente) - MODO HORIZONTAL
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
        .rotate = LV_DISP_ROT_90  // Forcar rotacao para modo horizontal (landscape)
    };
    
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    
    ESP_LOGI(TAG, "Display inicializado!");
    
    // Inicializa sistema operacional
    ui_os_init();
    ESP_LOGI(TAG, "Sistema Operacional inicializado!");
    
    // Inicializa sensores
    if (sensors_init()) {
        ESP_LOGI(TAG, "Sensores inicializados com sucesso!");
        ui_show_notification("Sensores conectados", "success");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar sensores!");
        ui_show_notification("Erro nos sensores", "error");
    }
    
    // Inicializa LiDAR
    lidar_config_t lidar_config = {
        .uart_num = LIDAR_UART_NUM,
        .tx_pin = LIDAR_TX_PIN,
        .rx_pin = LIDAR_RX_PIN,
        .baud_rate = LIDAR_BAUD_RATE,
        .timeout_ms = 1000
    };
    
    if (lidar_init(&lidar_config) == ESP_OK) {
        ESP_LOGI(TAG, "LiDAR TF Mini Plus inicializado com sucesso!");
        ui_show_notification("LiDAR conectado", "success");
    } else {
        ESP_LOGE(TAG, "Falha na inicializacao do LiDAR");
        ui_show_notification("Erro no LiDAR", "error");
    }
    
    ESP_LOGI(TAG, "Sistema pronto! Iniciando loop principal...");
    
    // Variaveis do sistema
    ui_system_state_t system_state = {0};
    uint32_t last_system_update = 0;
    uint32_t last_sensor_read = 0;
    
    // Loop principal
    while (1) {
        uint32_t current_time = esp_log_timestamp();
        
        // Atualiza sistema operacional a cada 100ms
        if (current_time - last_system_update >= 100) {
            last_system_update = current_time;
            ui_os_update();
            
            // Atualiza estado do sistema
            system_state.wifi_connected = false; // Implementar WiFi futuramente
            system_state.bluetooth_connected = false; // Implementar BT futuramente
            system_state.battery_level = 95; // Simular bateria por enquanto
            strcpy(system_state.current_time, "11 Oct 2025 17:20");
            
            ui_update_system_state(&system_state);
        }
        
        // Le sensores a cada 500ms
        if (current_time - last_sensor_read >= 500) {
            last_sensor_read = current_time;
            
            // Le dados dos sensores
            if (sensors_read_all(&sensor_data)) {
                // Atualiza interface com dados dos sensores
                ui_update_sensor_data(&sensor_data);
                
                // Log apenas quando necessário
                if (sensor_data.bno055_valid) {
                    ESP_LOGD(TAG, "BNO055 - P:%.1f R:%.1f Y:%.1f", 
                            sensor_data.pitch, sensor_data.roll, sensor_data.yaw);
                }
            } else {
                ESP_LOGW(TAG, "Falha ao ler sensores");
            }
        }
        
        // Pequeno delay para nao sobrecarregar CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
