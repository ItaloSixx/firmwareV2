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
#include "esp_sntp.h"
#include <sys/time.h>
#include "config.h"
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"

// Inclusao dos modulos do projeto
#include "interface/ui_os.h"
#include "interface/screens/settings/wifi/wifi_main.h"
#include "sensors/sensors.h"
#include "sensors/lidar_tf_mini.h"
#include "storage/sd_storage.h"
#include "supabase/supabase_client.h"

// Variaveis globais
static sensor_data_t sensor_data = {0};
static const char *TAG = "MAIN";

// Sincroniza data/hora via NTP se Wi-Fi estiver conectado
static void sync_time_once(void)
{
    if (!wifi_main_is_connected()) {
        ESP_LOGW(TAG, "Wi-Fi nao conectado, pulando sincronizacao NTP");
        return;
    }

    ESP_LOGI(TAG, "Iniciando sincronizacao NTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();

    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (retry < retry_count) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "Hora sincronizada: %02d/%02d/%04d %02d:%02d:%02d",
                 timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGW(TAG, "Falha ao sincronizar NTP");
    }
}

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

    // Inicializa o serviço Wi-Fi em background
    wifi_main_service_init();

    // Tentar sincronizar hora via NTP (se Wi-Fi já conectou)
    sync_time_once();
    
    // Inicializa cliente Supabase (após Wi-Fi estar configurado)
    if (supabase_init() == ESP_OK) {
        ESP_LOGI(TAG, "Cliente Supabase inicializado!");
    } else {
        ESP_LOGW(TAG, "Falha ao inicializar Supabase");
    }
    
    // Configurar timezone para UTC para evitar problemas de conversão
    setenv("TZ", "UTC", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone configurado para UTC");
    
    // Inicializa display (usando codigo ESP-IDF existente) - MODO HORIZONTAL
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
        .rotate = LV_DISP_ROT_270  // Inverter rotacao para modo horizontal invertido
    };
    
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    
    ESP_LOGI(TAG, "Display inicializado!");
    
    // Inicializa sistema operacional
    ui_os_init();
    ESP_LOGI(TAG, "Sistema Operacional inicializado!");

    // Monta cartão SD no boot
    if (sd_storage_init()) {
        ESP_LOGI(TAG, "Cartão SD montado em %s", SD_MOUNT_POINT);
        ui_show_notification("Cartão SD montado", "success");
    } else {
        ESP_LOGW(TAG, "Falha ao montar cartão SD");
        ui_show_notification("Cartão não detectado", "warning");
    }
    
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
            system_state.wifi_connected = wifi_main_is_connected();
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
