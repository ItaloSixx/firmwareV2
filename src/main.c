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
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"

// Inclusao dos modulos do projeto
#include "interface/ui_main.h"
#include "sensors/sensors.h"

// Variaveis globais
static sensor_data_t sensor_data = {0};
static uint32_t last_sensor_read = 0;
static const char *TAG = "MAIN";

/**
 * @brief Inicializa o sistema
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== JC3248W535EN - Teste BNO055 ===");
    ESP_LOGI(TAG, "Inicializando sistema...");
    
    // Inicializa display (usando codigo ESP-IDF existente)
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#endif
    };
    
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    
    ESP_LOGI(TAG, "Display inicializado!");
    
    // Inicializa interface
    ui_main_init();
    ESP_LOGI(TAG, "Interface criada!");
    
    // Inicializa sensores
    if (sensors_init()) {
        ESP_LOGI(TAG, "Sensores inicializados com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar sensores!");
    }
    
    ESP_LOGI(TAG, "Sistema pronto! Iniciando loop principal...");
    
    // Loop principal
    while (1) {
        // Le sensores a cada 500ms
        uint32_t current_time = esp_log_timestamp();
        if (current_time - last_sensor_read >= 500) {
            last_sensor_read = current_time;
            
            // Le dados dos sensores
            if (sensors_read_all(&sensor_data)) {
                // Atualiza interface com dados dos sensores
                ui_update_sensor_data(&sensor_data);
            } else {
                ESP_LOGW(TAG, "Falha ao ler sensores");
            }
        }
        
        // Pequeno delay para nao sobrecarregar CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
