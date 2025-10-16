/**
 * @file ui_os.c
 * @brief Sistema Operacional Principal - Implementação de compatibilidade
 * @author ItaloSixx
 * @date 2025
 */

#include "ui_os.h"
#include "ui_manager.h"
#include <esp_log.h>

static const char *TAG = "UI_OS_COMPAT";
static ui_manager_t *g_ui_manager = NULL;

void ui_os_init(void)
{
    ESP_LOGI(TAG, "Initializing UI OS (compatibility mode)");
    g_ui_manager = ui_manager_init();
    if (!g_ui_manager) {
        ESP_LOGE(TAG, "Failed to initialize UI manager");
    }

}

void ui_set_screen(ui_screen_t screen)
{
    if (!g_ui_manager) {
        ESP_LOGW(TAG, "UI manager not initialized");
        return;
    }
    
    ui_manager_set_screen(g_ui_manager, screen);
}

ui_screen_t ui_get_current_screen(void)
{
    if (!g_ui_manager) {
        ESP_LOGW(TAG, "UI manager not initialized");
        return UI_SCREEN_HOME;
    }
    
    return ui_manager_get_current_screen(g_ui_manager);
}

void ui_os_update(void)
{
    if (!g_ui_manager) {
        return;
    }
    
    ui_manager_update(g_ui_manager);
}

void ui_update_sensor_data(const sensor_data_t *data)
{
    if (!g_ui_manager || !data) {
        return;
    }
    
    ui_manager_update_sensor_data(g_ui_manager, data);
}

void ui_update_system_state(const ui_system_state_t *state)
{
    if (!g_ui_manager || !state) {
        return;
    }
    
    ui_manager_update_system_state(g_ui_manager, state);
}

void ui_update_system_stats(const ui_system_stats_t *stats)
{
    if (!g_ui_manager || !stats) {
        return;
    }
    
    ui_manager_update_system_stats(g_ui_manager, stats);
}

void ui_show_notification(const char *message, const char *type)
{
    if (!g_ui_manager || !message) {
        return;
    }
    
    ui_manager_show_notification(g_ui_manager, message, type);
}