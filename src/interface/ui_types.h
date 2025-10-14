/**
 * @file ui_types.h
 * @brief Tipos comuns da interface - Definições centralizadas
 * @author ItaloSixx
 * @date 2025
 */

#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ENUMERAÇÕES
// =============================================================================

typedef enum {
    UI_SCREEN_HOME = 0,
    UI_SCREEN_SENSORS,
    UI_SCREEN_MEASUREMENT,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_ABOUT,
    UI_SCREEN_COUNT
} ui_screen_t;

// =============================================================================
// ESTRUTURAS DE DADOS
// =============================================================================

typedef struct {
    bool wifi_connected;
    bool bluetooth_connected;
    int battery_level;
    char current_time[32];
} ui_system_state_t;

typedef struct {
    uint32_t free_heap;
    uint32_t min_free_heap;
    float cpu_usage;
    uint32_t uptime_ms;
} ui_system_stats_t;

#ifdef __cplusplus
}
#endif

#endif // UI_TYPES_H