/**
 * @file screen_home.h
 * @brief Tela principal/home da interface
 * @author ItaloSixx
 * @date 2025
 */

#ifndef SCREEN_HOME_H
#define SCREEN_HOME_H

#include "lvgl.h"
#include "../components/ui_components.h"

/**
 * @brief Cria a tela home
 */
lv_obj_t *screen_home_create(lv_obj_t *parent);

/**
 * @brief Define o callback de navegação
 */
void screen_home_set_navigation_callback(void (*callback)(ui_screen_t screen));

/**
 * @brief Atualiza dados da tela home
 */
void screen_home_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela home
 */
void screen_home_destroy(lv_obj_t *screen);

#endif // SCREEN_HOME_H