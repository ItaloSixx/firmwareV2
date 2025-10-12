/**
 * @file settings_main.h
 * @brief Tela principal de configurações do sistema
 * @author ItaloSixx
 * @date 2025
 */

#ifndef SETTINGS_MAIN_H
#define SETTINGS_MAIN_H

#include "lvgl.h"
#include "../../components/ui_components.h"

// Forward declaration para callback de navegação
typedef void (*settings_navigation_cb_t)(const char *screen_name);

/**
 * @brief Cria a tela principal de configurações
 */
lv_obj_t *settings_main_create(lv_obj_t *parent);

/**
 * @brief Define o callback de navegação para subtelas
 */
void settings_main_set_navigation_callback(settings_navigation_cb_t callback);

/**
 * @brief Atualiza dados da tela de configurações
 */
void settings_main_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela de configurações
 */
void settings_main_destroy(lv_obj_t *screen);

/**
 * @brief Funções para acessar configurações externamente
 */
uint8_t settings_main_get_brightness(void);
bool settings_main_get_night_mode(void);
bool settings_main_get_wifi_enabled(void);
bool settings_main_get_bluetooth_enabled(void);

void settings_main_set_brightness(uint8_t value);
void settings_main_set_night_mode(bool enabled);
void settings_main_set_wifi_enabled(bool enabled);
void settings_main_set_bluetooth_enabled(bool enabled);

#endif // SETTINGS_MAIN_H