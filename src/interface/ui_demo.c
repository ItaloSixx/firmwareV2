/**
 * @file ui_demo.c
 * @brief Demonstração da UI modular
 * @author ItaloSixx
 * @date 2025
 */

#include "ui_os.h"
#include "sensors/sensors.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "UI_DEMO";

// Dados simulados para demonstração
static sensor_data_t demo_sensor_data = {
    .pitch = 10.5f,
    .roll = -5.2f,
    .yaw = 180.0f,
    .roll_offset = 0.0f,
    .bno055_valid = true,
    .lidar_distance = 1200,
    .lidar_valid = true,
    .battery_voltage = 3.7f,
    .low_battery = false,
    .timestamp = 0
};

static ui_system_state_t demo_system_state = {
    .wifi_connected = true,
    .bluetooth_connected = false,
    .battery_level = 85,
    .current_time = "11 Oct 2025 17:15"
};

static ui_system_stats_t demo_system_stats = {
    .free_heap = 128000,
    .min_free_heap = 96000,
    .cpu_usage = 45.2f,
    .uptime_ms = 3600000
};

void ui_demo_init(void)
{
    ESP_LOGI(TAG, "Initializing UI Demo");
    
    // Inicializar UI
    ui_os_init();
    
    // Configurar dados iniciais
    ui_update_sensor_data(&demo_sensor_data);
    ui_update_system_state(&demo_system_state);
    ui_update_system_stats(&demo_system_stats);
    
    ESP_LOGI(TAG, "UI Demo initialized successfully");
}

void ui_demo_update_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1 segundo
    
    uint32_t counter = 0;
    
    while (1) {
        // Simular mudanças nos dados dos sensores
        demo_sensor_data.pitch += (float)(rand() % 10 - 5) * 0.1f;
        demo_sensor_data.roll += (float)(rand() % 10 - 5) * 0.1f;
        demo_sensor_data.yaw += (float)(rand() % 10 - 5) * 0.5f;
        demo_sensor_data.lidar_distance = 1000 + (rand() % 500);
        demo_sensor_data.battery_voltage = 3.3f + (float)(rand() % 100) * 0.01f;
        demo_sensor_data.timestamp = counter++;
        
        // Simular mudanças no estado do sistema
        demo_system_stats.free_heap = 100000 + (rand() % 50000);
        demo_system_stats.cpu_usage = 30.0f + (float)(rand() % 40);
        demo_system_stats.uptime_ms += 1000;
        
        // Atualizar UI
        ui_update_sensor_data(&demo_sensor_data);
        ui_update_system_stats(&demo_system_stats);
        ui_os_update();
        
        // Mostrar notificação ocasionalmente
        if (counter % 30 == 0) {
            ui_show_notification("System running normally", "info");
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void ui_demo_navigation_test(void)
{
    ESP_LOGI(TAG, "Testing navigation between screens");
    
    // Navegar por todas as telas
    for (int i = 0; i < UI_SCREEN_COUNT; i++) {
        ESP_LOGI(TAG, "Switching to screen %d", i);
        ui_set_screen((ui_screen_t)i);
        vTaskDelay(pdMS_TO_TICKS(3000)); // 3 segundos em cada tela
    }
    
    // Voltar para home
    ui_set_screen(UI_SCREEN_HOME);
    ESP_LOGI(TAG, "Navigation test completed");
}