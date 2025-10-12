/**
 * @file screen_sensors.h
 * @brief Tela de monitoramento de sensores
 * @author ItaloSixx
 * @date 2025
 */

#ifndef SCREEN_SENSORS_H
#define SCREEN_SENSORS_H

#include "lvgl.h"
#include "../components/ui_components.h"
#include "../../sensors/sensors.h"

/**
 * @brief Cria a tela de sensores
 */
lv_obj_t *screen_sensors_create(lv_obj_t *parent);

/**
 * @brief Atualiza dados dos sensores na tela
 */
void screen_sensors_update(lv_obj_t *screen, const sensor_data_t *data);

/**
 * @brief Destrói a tela de sensores
 */
void screen_sensors_destroy(lv_obj_t *screen);

#endif // SCREEN_SENSORS_H