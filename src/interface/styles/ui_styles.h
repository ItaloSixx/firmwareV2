/**
 * @file ui_styles.h
 * @brief Definições de estilos e constantes visuais
 * @author ItaloSixx
 * @date 2025
 */

#ifndef UI_STYLES_H
#define UI_STYLES_H

#include "lvgl.h"

// =============================================================================
// CORES DO TEMA
// =============================================================================

// Tema Light Material Design
#define UI_COLOR_PRIMARY           0x6750A4
#define UI_COLOR_PRIMARY_VARIANT   0x4F378B
#define UI_COLOR_SECONDARY         0x625B71
#define UI_COLOR_BACKGROUND        0xFFFBFE
#define UI_COLOR_SURFACE           0xFFFFFF
#define UI_COLOR_ERROR             0xBA1A1A
#define UI_COLOR_ON_PRIMARY        0xFFFFFF
#define UI_COLOR_ON_SECONDARY      0xFFFFFF
#define UI_COLOR_ON_BACKGROUND     0x1C1B1F
#define UI_COLOR_ON_SURFACE        0x1C1B1F
#define UI_COLOR_ON_ERROR          0xFFFFFF

// Cores auxiliares
#define UI_COLOR_SUCCESS           0x4CAF50
#define UI_COLOR_WARNING           0xFF9800
#define UI_COLOR_INFO              0x2196F3
#define UI_COLOR_DARK              0x2C3E50
#define UI_COLOR_LIGHT             0xF8F9FA

// =============================================================================
// DIMENSÕES E ESPAÇAMENTOS
// =============================================================================

// Dimensões da tela
#define UI_SCREEN_WIDTH            480
#define UI_SCREEN_HEIGHT           320
#define UI_STATUS_BAR_HEIGHT       30
#define UI_NAV_BAR_HEIGHT          50
#define UI_CONTENT_HEIGHT          (UI_SCREEN_HEIGHT - UI_STATUS_BAR_HEIGHT - UI_NAV_BAR_HEIGHT)

// Espaçamentos
#define UI_MARGIN_SMALL            4
#define UI_MARGIN_MEDIUM           8
#define UI_MARGIN_LARGE            16
#define UI_MARGIN_XLARGE           24

// Raios de borda
#define UI_RADIUS_SMALL            4
#define UI_RADIUS_MEDIUM           8
#define UI_RADIUS_LARGE            12
#define UI_RADIUS_XLARGE           16

// Tamanhos de fonte
#define UI_FONT_SMALL              &lv_font_montserrat_10
#define UI_FONT_MEDIUM             &lv_font_montserrat_12
#define UI_FONT_LARGE              &lv_font_montserrat_14
#define UI_FONT_XLARGE             &lv_font_montserrat_16

// =============================================================================
// FUNÇÕES DE ESTILO
// =============================================================================

/**
 * @brief Aplica estilo de card moderno
 */
void ui_apply_card_style(lv_obj_t *obj);

/**
 * @brief Aplica estilo de botão primário
 */
void ui_apply_button_primary_style(lv_obj_t *obj);

/**
 * @brief Aplica estilo de botão secundário
 */
void ui_apply_button_secondary_style(lv_obj_t *obj);

/**
 * @brief Aplica estilo de texto título
 */
void ui_apply_title_style(lv_obj_t *obj);

/**
 * @brief Aplica estilo de texto corpo
 */
void ui_apply_body_style(lv_obj_t *obj);

/**
 * @brief Aplica estilo de container principal
 */
void ui_apply_container_style(lv_obj_t *obj);

#endif // UI_STYLES_H