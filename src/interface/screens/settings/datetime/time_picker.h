/**
 * @file time_picker.h
 * @brief Seletor de hora
 * @author ItaloSixx
 * @date 2025
 */

#ifndef TIME_PICKER_H
#define TIME_PICKER_H

#include "lvgl.h"
#include "../../../components/ui_components.h"

// Forward declaration para callback de navegação
typedef void (*time_picker_navigation_cb_t)(const char *screen_name);

/**
 * @brief Cria a tela do seletor de hora
 */
lv_obj_t *time_picker_create(lv_obj_t *parent);

/**
 * @brief Define o callback de navegação
 */
void time_picker_set_navigation_callback(time_picker_navigation_cb_t callback);

/**
 * @brief Atualiza dados do seletor de hora
 */
void time_picker_update(lv_obj_t *screen);

/**
 * @brief Destrói o seletor de hora
 */
void time_picker_destroy(lv_obj_t *screen);

/**
 * @brief Define a hora inicial
 */
void time_picker_set_initial(int hour, int minute);

#endif // TIME_PICKER_H