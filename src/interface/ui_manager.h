/**
 * @file ui_manager.h
 * @brief Gerenciador principal da interface modularizada
 * @author ItaloSixx
 * @date 2025
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"
#include "ui_types.h"
#include "components/ui_components.h"
#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include "screens/screen_settings.h"
#include "screens/screen_about.h"
#include "../sensors/sensors.h"

// =============================================================================
// TIPOS DE DADOS
// =============================================================================

typedef struct {
    lv_obj_t *main_container;
    lv_obj_t *content_area;
    lv_obj_t *current_screen;
    
    ui_status_bar_t *status_bar;
    ui_nav_bar_t *nav_bar;
    
    ui_screen_t active_screen;
    
    // Ponteiros para telas
    lv_obj_t *screens[UI_SCREEN_COUNT];
    
    // Sub-telas ativas (para configurações, etc.)
    lv_obj_t *current_subscreen;
    
    // Estado do sistema
    ui_system_state_t system_state;
    ui_system_stats_t system_stats;
    sensor_data_t sensor_data;
    
} ui_manager_t;

// =============================================================================
// FUNÇÕES PRINCIPAIS
// =============================================================================

/**
 * @brief Inicializa o gerenciador de UI
 */
ui_manager_t *ui_manager_init(void);

/**
 * @brief Atualiza a interface (chamada periodicamente)
 */
void ui_manager_update(ui_manager_t *manager);

/**
 * @brief Navega para uma tela específica
 */
void ui_manager_set_screen(ui_manager_t *manager, ui_screen_t screen);

/**
 * @brief Obtém a tela atual
 */
ui_screen_t ui_manager_get_current_screen(ui_manager_t *manager);

/**
 * @brief Atualiza dados dos sensores
 */
void ui_manager_update_sensor_data(ui_manager_t *manager, const sensor_data_t *data);

/**
 * @brief Atualiza estado do sistema
 */
void ui_manager_update_system_state(ui_manager_t *manager, const ui_system_state_t *state);

/**
 * @brief Atualiza estatísticas do sistema
 */
void ui_manager_update_system_stats(ui_manager_t *manager, const ui_system_stats_t *stats);

/**
 * @brief Mostra uma notificação
 */
void ui_manager_show_notification(ui_manager_t *manager, const char *message, const char *type);

/**
 * @brief Navega para uma sub-tela de configurações
 */
void ui_manager_navigate_to_settings(const char *screen_name);

/**
 * @brief Libera recursos do gerenciador
 */
void ui_manager_destroy(ui_manager_t *manager);

#endif // UI_MANAGER_H