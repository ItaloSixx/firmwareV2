/**
 * @file ui_components.h
 * @brief Componentes reutilizáveis da interface
 * @author ItaloSixx
 * @date 2025
 */

#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "lvgl.h"
#include "../styles/ui_styles.h"

// =============================================================================
// TIPOS DE DADOS
// =============================================================================

#include "../ui_types.h"

typedef struct {
    lv_obj_t *status_bar;
    lv_obj_t *time_label;
    lv_obj_t *wifi_icon;
    lv_obj_t *call_icon;
    lv_obj_t *edit_icon;
    lv_obj_t *folder_icon;
} ui_status_bar_t;

typedef struct {
    lv_obj_t *nav_bar;
    lv_obj_t *nav_buttons[UI_SCREEN_COUNT];
    ui_screen_t current_screen;
} ui_nav_bar_t;

// =============================================================================
// FUNÇÕES DOS COMPONENTES
// =============================================================================

/**
 * @brief Cria a barra de status superior
 */
ui_status_bar_t *ui_create_status_bar(lv_obj_t *parent);

/**
 * @brief Atualiza o horário na barra de status
 */
void ui_update_status_time(ui_status_bar_t *status_bar, const char *time_str);

/**
 * @brief Cria a barra de navegação inferior
 */
ui_nav_bar_t *ui_create_nav_bar(lv_obj_t *parent, void (*nav_callback)(ui_screen_t screen));

/**
 * @brief Atualiza o botão ativo na navegação
 */
void ui_update_nav_active(ui_nav_bar_t *nav_bar, ui_screen_t active_screen);

/**
 * @brief Cria um card container
 */
lv_obj_t *ui_create_card(lv_obj_t *parent, int width, int height);

/**
 * @brief Cria um botão com estilo primário
 */
lv_obj_t *ui_create_button_primary(lv_obj_t *parent, const char *text, lv_event_cb_t callback);

/**
 * @brief Cria um botão com estilo secundário
 */
lv_obj_t *ui_create_button_secondary(lv_obj_t *parent, const char *text, lv_event_cb_t callback);

/**
 * @brief Cria um título
 */
lv_obj_t *ui_create_title(lv_obj_t *parent, const char *text);

/**
 * @brief Cria texto corpo
 */
lv_obj_t *ui_create_body_text(lv_obj_t *parent, const char *text);

/**
 * @brief Cria área de conteúdo principal
 */
lv_obj_t *ui_create_content_area(lv_obj_t *parent);

/**
 * @brief Libera recursos de uma barra de status
 */
void ui_destroy_status_bar(ui_status_bar_t *status_bar);

/**
 * @brief Libera recursos de uma barra de navegação
 */
void ui_destroy_nav_bar(ui_nav_bar_t *nav_bar);

#endif // UI_COMPONENTS_H