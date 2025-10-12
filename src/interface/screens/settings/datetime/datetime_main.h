/**
 * @file datetime_main.h
 * @brief Tela principal de configurações de data e hora
 * @author ItaloSixx
 * @date 2025
 */

#ifndef DATETIME_MAIN_H
#define DATETIME_MAIN_H

#include "lvgl.h"
#include "../../../components/ui_components.h"

// Forward declaration para callback de navegação
typedef void (*datetime_navigation_cb_t)(const char *screen_name);
typedef void (*datetime_back_cb_t)(void);

/**
 * @brief Cria a tela principal de configurações de data/hora
 */
lv_obj_t *datetime_main_create(lv_obj_t *parent);

/**
 * @brief Define o callback de navegação para subtelas
 */
void datetime_main_set_navigation_callback(datetime_navigation_cb_t callback);

/**
 * @brief Define o callback para voltar à tela anterior
 */
void datetime_main_set_back_callback(datetime_back_cb_t callback);

/**
 * @brief Atualiza dados da tela de configurações de data/hora
 */
void datetime_main_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela de configurações de data/hora
 */
void datetime_main_destroy(lv_obj_t *screen);

/**
 * @brief Obtém os valores atuais de data/hora
 */
void datetime_get_current(int *hour, int *minute, int *day, int *month, int *year);

/**
 * @brief Define os valores de data/hora
 */
void datetime_set_current(int hour, int minute, int day, int month, int year);

/**
 * @brief Define apenas a hora
 */
void datetime_main_set_time(int hour, int minute);

/**
 * @brief Define apenas a data
 */
void datetime_main_set_date(int day, int month, int year);

#endif // DATETIME_MAIN_H