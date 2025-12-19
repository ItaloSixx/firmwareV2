/**
 * @file measurement_main.h
 * @brief Sistema de medição de altura de plantas usando LiDAR TF Mini Plus
 * @author ItaloSixx
 * @date 2025
 */

#ifndef MEASUREMENT_MAIN_H
#define MEASUREMENT_MAIN_H

#include "lvgl.h"
#include "../../components/ui_components.h"

// Estrutura para armazenar dados de medição
typedef struct {
    float distance_horizontal;  // Distância horizontal ao centro da planta (cm)
    float distance_to_top;     // Distância diagonal ao topo (cm)
    float distance_to_base;    // Distância diagonal à base (cm)
    float height_top;          // h1 - altura do topo (cm)
    float height_base;         // h2 - altura da base (cm)
    float total_height;        // Altura total da planta (cm)
    bool measurement_valid;    // Se a medição é válida
    char timestamp[32];        // Timestamp da medição
} plant_measurement_t;

// Modalidades de medição
typedef enum {
    MEASUREMENT_MODE_FULL = 0,   // Medição completa (3 leituras)
    MEASUREMENT_MODE_SINGLE      // Medição única (1 leitura)
} measurement_mode_t;

// Estados do processo de medição
typedef enum {
    PLANT_MEASUREMENT_STATE_IDLE,
    PLANT_MEASUREMENT_STATE_HORIZONTAL,
    PLANT_MEASUREMENT_STATE_TOP,
    PLANT_MEASUREMENT_STATE_BASE,
    PLANT_MEASUREMENT_STATE_CALCULATING,
    PLANT_MEASUREMENT_STATE_COMPLETE
} plant_measurement_state_t;

/**
 * @brief Cria a tela principal de medição
 */
lv_obj_t *measurement_main_create(lv_obj_t *parent);

/**
 * @brief Atualiza dados da tela de medição
 */
void measurement_main_update(lv_obj_t *screen);

/**
 * @brief Destrói a tela de medição
 */
void measurement_main_destroy(lv_obj_t *screen);

/**
 * @brief Inicia o processo de medição
 */
void measurement_start_process(void);

/**
 * @brief Para o processo de medição
 */
void measurement_stop_process(void);

/**
 * @brief Obtém o estado atual da medição
 */
plant_measurement_state_t measurement_get_state(void);

/**
 * @brief Obtém os dados da última medição
 */
plant_measurement_t measurement_get_data(void);

/**
 * @brief Salva a medição atual
 */
bool measurement_save_current(void);

/**
 * @brief Carrega histórico de medições
 */
bool measurement_load_history(void);

/**
 * @brief Define o modo de medição (completa ou única)
 */
void measurement_set_mode(measurement_mode_t mode);

/**
 * @brief Obtém o modo de medição atual
 */
measurement_mode_t measurement_get_mode(void);

#endif // MEASUREMENT_MAIN_H