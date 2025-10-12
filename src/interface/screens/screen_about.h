/**
 * @file screen_about.h
 * @brief Tela sobre o sistema
 * @author ItaloSixx
 * @date 2025
 */

#ifndef SCREEN_ABOUT_H
#define SCREEN_ABOUT_H

#include "lvgl.h"
#include "../components/ui_components.h"

/**
 * @brief Cria a tela sobre
 */
lv_obj_t *screen_about_create(lv_obj_t *parent);

/**
 * @brief Atualiza dados da tela sobre
 */
void screen_about_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela sobre
 */
void screen_about_destroy(lv_obj_t *screen);

#endif // SCREEN_ABOUT_H