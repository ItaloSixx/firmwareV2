#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converte temperatura de Celsius para Fahrenheit
 * @param celsius Temperatura em Celsius
 * @return Temperatura em Fahrenheit
 */
float utils_celsius_to_fahrenheit(float celsius);

/**
 * @brief Converte temperatura de Fahrenheit para Celsius
 * @param fahrenheit Temperatura em Fahrenheit
 * @return Temperatura em Celsius
 */
float utils_fahrenheit_to_celsius(float fahrenheit);

/**
 * @brief Formata uma string com informações de sistema
 * @param buffer Buffer para armazenar a string formatada
 * @param buffer_size Tamanho do buffer
 * @return Número de caracteres escritos
 */
int utils_format_system_info(char *buffer, size_t buffer_size);

/**
 * @brief Obtém timestamp atual em formato string
 * @param buffer Buffer para armazenar o timestamp
 * @param buffer_size Tamanho do buffer
 * @return true se formatação foi bem-sucedida
 */
bool utils_get_timestamp_string(char *buffer, size_t buffer_size);

/**
 * @brief Mapeia um valor de um range para outro
 * @param value Valor a ser mapeado
 * @param from_min Valor mínimo do range original
 * @param from_max Valor máximo do range original
 * @param to_min Valor mínimo do range de destino
 * @param to_max Valor máximo do range de destino
 * @return Valor mapeado
 */
float utils_map_value(float value, float from_min, float from_max, float to_min, float to_max);

/**
 * @brief Limita um valor dentro de um range específico
 * @param value Valor a ser limitado
 * @param min_val Valor mínimo
 * @param max_val Valor máximo
 * @return Valor limitado
 */
float utils_clamp(float value, float min_val, float max_val);

/**
 * @brief Calcula a média de um array de valores
 * @param values Array de valores
 * @param count Número de valores no array
 * @return Média dos valores
 */
float utils_calculate_average(const float *values, size_t count);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
