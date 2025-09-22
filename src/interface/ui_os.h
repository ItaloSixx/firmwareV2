/**
 * @file ui_os.h
 * @brief Sistema Operacional Principal - Interface Grafica ESP32-S3
 * @author Embedded OS Team
 * @date 2025
 */

#ifndef UI_OS_H
#define UI_OS_H

#include "lvgl.h"
#include "../sensors/sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONSTANTES DE DESIGN MODERNO
// =============================================================================

// Cores do tema moderno (Material You)
#define UI_COLOR_PRIMARY           0x6750A4    // Purple modern
#define UI_COLOR_PRIMARY_VARIANT   0x4F378B    // Purple dark
#define UI_COLOR_SECONDARY         0x625B71    // Purple gray
#define UI_COLOR_BACKGROUND        0x121212    // Dark background
#define UI_COLOR_SURFACE           0x1C1B1F    // Card surface
#define UI_COLOR_ON_PRIMARY        0xFFFFFF    // White text
#define UI_COLOR_ON_BACKGROUND     0xE6E1E5    // Light text
#define UI_COLOR_SUCCESS           0x4CAF50    // Green
#define UI_COLOR_WARNING           0xFF9800    // Orange
#define UI_COLOR_ERROR             0xF44336    // Red
#define UI_COLOR_INFO              0x2196F3    // Blue
#define UI_COLOR_ACCENT            0x03DAC6    // Teal accent

// Dimensoes de layout (landscape 480x320)
#define UI_SCREEN_WIDTH            480
#define UI_SCREEN_HEIGHT           320
#define UI_STATUS_BAR_HEIGHT       32
#define UI_NAV_BAR_HEIGHT          56
#define UI_CONTENT_HEIGHT          (UI_SCREEN_HEIGHT - UI_STATUS_BAR_HEIGHT - UI_NAV_BAR_HEIGHT)
#define UI_CONTENT_WIDTH           UI_SCREEN_WIDTH
#define UI_MARGIN_SMALL            8
#define UI_MARGIN_MEDIUM           16
#define UI_MARGIN_LARGE            24
#define UI_RADIUS_SMALL            6
#define UI_RADIUS_MEDIUM           12
#define UI_RADIUS_LARGE            16

// =============================================================================
// ENUMS E ESTRUTURAS
// =============================================================================

/**
 * @brief Enum das telas disponíveis no sistema
 */
typedef enum {
    UI_SCREEN_HOME = 0,
    UI_SCREEN_SENSORS,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_ABOUT,
    UI_SCREEN_COUNT
} ui_screen_t;

/**
 * @brief Estrutura de estado do sistema UI
 */
typedef struct {
    ui_screen_t current_screen;
    bool wifi_connected;
    bool bluetooth_enabled;
    uint8_t battery_level;
    bool low_battery;
    char time_str[16];
    uint32_t notification_count;
} ui_system_state_t;

/**
 * @brief Estrutura de estatísticas do sistema
 */
typedef struct {
    uint32_t uptime_ms;
    uint32_t free_heap;
    uint32_t min_free_heap;
    float cpu_usage;
    uint16_t task_count;
} ui_system_stats_t;

// =============================================================================
// FUNÇÕES PRINCIPAIS
// =============================================================================

/**
 * @brief Inicializa o sistema operacional completo
 */
void ui_os_init(void);

/**
 * @brief Atualiza o sistema (chamado no loop principal)
 */
void ui_os_update(void);

/**
 * @brief Muda para uma tela específica
 * @param screen Tela de destino
 */
void ui_set_screen(ui_screen_t screen);

/**
 * @brief Obtém a tela atual
 * @return Tela atual
 */
ui_screen_t ui_get_current_screen(void);

// =============================================================================
// FUNÇÕES DE ATUALIZAÇÃO DE DADOS
// =============================================================================

/**
 * @brief Atualiza dados dos sensores
 * @param data Dados dos sensores
 */
void ui_update_sensor_data(const sensor_data_t *data);

/**
 * @brief Atualiza estado do sistema
 * @param state Estado do sistema
 */
void ui_update_system_state(const ui_system_state_t *state);

/**
 * @brief Atualiza estatísticas do sistema
 * @param stats Estatísticas do sistema
 */
void ui_update_system_stats(const ui_system_stats_t *stats);

// =============================================================================
// FUNÇÕES DE NOTIFICAÇÃO
// =============================================================================

/**
 * @brief Mostra notificação temporária
 * @param message Mensagem da notificação
 * @param type Tipo (success, warning, error, info)
 */
void ui_show_notification(const char *message, const char *type);

/**
 * @brief Atualiza contador de notificações
 * @param count Numero de notificacoes
 */
void ui_update_notification_count(uint32_t count);

#ifdef __cplusplus
}
#endif

#endif // UI_OS_H