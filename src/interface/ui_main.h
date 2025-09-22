#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <lvgl.h>
#include "sensors/sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa a interface principal do usuário
 */
void ui_main_init(void);

/**
 * @brief Atualiza o texto do label de dados do sensor
 * @param text Novo texto a ser exibido
 */
void ui_update_label(const char* text);

/**
 * @brief Atualiza status de conexão do BNO055
 * @param bno055_connected true se BNO055 está conectado
 */
void ui_update_connection_status(bool bno055_connected);

/**
 * @brief Atualiza interface com dados dos sensores
 * @param data Estrutura com dados dos sensores
 */
void ui_update_sensor_data(const sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // UI_MAIN_H
