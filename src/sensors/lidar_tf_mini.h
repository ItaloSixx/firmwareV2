/**
 * @file lidar_tf_mini.h
 * @brief Driver para LiDAR TF Mini Plus
 * @author ItaloSixx  
 * @date 2025
 */

#ifndef LIDAR_TF_MINI_H
#define LIDAR_TF_MINI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Estrutura de dados do LiDAR
typedef struct {
    uint16_t distance;      // Distância em centímetros
    uint16_t strength;      // Força do sinal
    uint16_t temperature;   // Temperatura interna (°C)
    bool valid;             // Dados válidos
    uint32_t timestamp;     // Timestamp da última leitura
} lidar_data_t;

// Códigos de erro específicos do LiDAR
typedef enum {
    LIDAR_OK = 0,
    LIDAR_ERROR_INIT,
    LIDAR_ERROR_TIMEOUT,
    LIDAR_ERROR_CHECKSUM,
    LIDAR_ERROR_INVALID_DATA,
    LIDAR_ERROR_COMMUNICATION
} lidar_error_t;

// Configuração do LiDAR
typedef struct {
    int uart_num;
    int tx_pin;
    int rx_pin;
    int baud_rate;
    int timeout_ms;
} lidar_config_t;

/**
 * @brief Inicializa o LiDAR TF Mini Plus
 * @param config Configuração do UART
 * @return ESP_OK se sucesso, erro caso contrário
 */
esp_err_t lidar_init(const lidar_config_t *config);

/**
 * @brief Deinicializa o LiDAR
 */
void lidar_deinit(void);

/**
 * @brief Lê dados do LiDAR (função bloqueante)
 * @param data Ponteiro para estrutura de dados
 * @return LIDAR_OK se sucesso, código de erro caso contrário
 */
lidar_error_t lidar_read_blocking(lidar_data_t *data);

/**
 * @brief Lê dados do LiDAR (função não-bloqueante)
 * @param data Ponteiro para estrutura de dados
 * @return LIDAR_OK se sucesso, código de erro caso contrário
 */
lidar_error_t lidar_read_async(lidar_data_t *data);

/**
 * @brief Verifica se há dados disponíveis
 * @return true se há dados disponíveis
 */
bool lidar_data_available(void);

/**
 * @brief Obtém a última leitura válida
 * @param data Ponteiro para estrutura de dados
 * @return true se dados válidos disponíveis
 */
bool lidar_get_last_reading(lidar_data_t *data);

/**
 * @brief Configura a frequência de medição (1-1000Hz)
 * @param freq_hz Frequência em Hz
 * @return LIDAR_OK se sucesso
 */
lidar_error_t lidar_set_frequency(uint16_t freq_hz);

/**
 * @brief Redefine o LiDAR para configurações padrão
 * @return LIDAR_OK se sucesso
 */
lidar_error_t lidar_reset(void);

/**
 * @brief Converte código de erro para string
 * @param error Código de erro
 * @return String descritiva do erro
 */
const char* lidar_error_to_string(lidar_error_t error);

#ifdef __cplusplus
}
#endif

#endif // LIDAR_TF_MINI_H