/**
 * @file screen_settings.h
 * @brief Tela de configurações do sistema
 * @author ItaloSixx
 * @date 2025
 */

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include "lvgl.h"
#include "../components/ui_components.h"

/**
 * @brief Cria a tela de configurações
 */
lv_obj_t *screen_settings_create(lv_obj_t *parent);

/**
 * @brief Atualiza dados da tela de configurações
 */
void screen_settings_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela de configurações
 */
void screen_settings_destroy(lv_obj_t *screen);

/**
 * @brief Funções para acessar configurações externamente
 */
uint8_t settings_get_brightness(void);
bool settings_get_night_mode(void);
bool settings_get_wifi_enabled(void);
bool settings_get_bluetooth_enabled(void);

void settings_set_brightness(uint8_t value);
void settings_set_night_mode(bool enabled);
void settings_set_wifi_enabled(bool enabled);
void settings_set_bluetooth_enabled(bool enabled);

#endif // SCREEN_SETTINGS_H