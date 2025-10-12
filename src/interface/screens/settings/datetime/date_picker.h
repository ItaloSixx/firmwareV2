/**
 * @file date_picker.h
 * @brief Seletor de data
 * @author ItaloSixx
 * @date 2025
 */

#ifndef DATE_PICKER_H
#define DATE_PICKER_H

#include "lvgl.h"
#include "../../../components/ui_components.h"

// Forward declaration para callback de navegação
typedef void (*date_picker_navigation_cb_t)(const char *screen_name);

/**
 * @brief Cria a tela do seletor de data
 */
lv_obj_t *date_picker_create(lv_obj_t *parent);

/**
 * @brief Define o callback de navegação
 */
void date_picker_set_navigation_callback(date_picker_navigation_cb_t callback);

/**
 * @brief Atualiza dados do seletor de data
 */
void date_picker_update(lv_obj_t *screen);

/**
 * @brief Destrói o seletor de data
 */
void date_picker_destroy(lv_obj_t *screen);

/**
 * @brief Define a data inicial
 */
void date_picker_set_initial(int day, int month, int year);

#endif // DATE_PICKER_H