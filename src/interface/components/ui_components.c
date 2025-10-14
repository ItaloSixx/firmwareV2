/**
 * @file ui_components.c
 * @brief Implementação dos componentes reutilizáveis
 * @author ItaloSixx
 * @date 2025
 */

#include "ui_components.h"
#include <stdlib.h>
#include <string.h>

// =============================================================================
// CALLBACKS INTERNOS
// =============================================================================

static void nav_button_event_cb(lv_event_t *e)
{
    ui_screen_t screen = (ui_screen_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    void (*callback)(ui_screen_t) = (void(*)(ui_screen_t))lv_obj_get_user_data(btn);
    
    if (callback) {
        callback(screen);
    }
}

// =============================================================================
// BARRA DE STATUS
// =============================================================================

ui_status_bar_t *ui_create_status_bar(lv_obj_t *parent)
{
    ui_status_bar_t *status_bar = malloc(sizeof(ui_status_bar_t));
    if (!status_bar) return NULL;
    
    // Container principal
    status_bar->status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar->status_bar, UI_SCREEN_WIDTH, UI_STATUS_BAR_HEIGHT);
    lv_obj_align(status_bar->status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar->status_bar, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar->status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar->status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar->status_bar, UI_MARGIN_SMALL, LV_PART_MAIN);
    
    // Desabilitar scroll em todas as direções
    lv_obj_clear_flag(status_bar->status_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // Label de horário
    status_bar->time_label = lv_label_create(status_bar->status_bar);
    lv_label_set_text(status_bar->time_label, "10 Apr 2020 15:36");
    lv_obj_set_style_text_color(status_bar->time_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_bar->time_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(status_bar->time_label, LV_ALIGN_RIGHT_MID, -UI_MARGIN_MEDIUM, 0);
    
    // Ícones de status (lado esquerdo)
    status_bar->wifi_icon = lv_label_create(status_bar->status_bar);
    lv_label_set_text(status_bar->wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_bar->wifi_icon, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_bar->wifi_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(status_bar->wifi_icon, LV_ALIGN_LEFT_MID, UI_MARGIN_MEDIUM, 0);
    
    status_bar->call_icon = lv_label_create(status_bar->status_bar);
    lv_label_set_text(status_bar->call_icon, LV_SYMBOL_CALL);
    lv_obj_set_style_text_color(status_bar->call_icon, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_bar->call_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(status_bar->call_icon, LV_ALIGN_LEFT_MID, UI_MARGIN_MEDIUM + 27, 0);
    
    status_bar->edit_icon = lv_label_create(status_bar->status_bar);
    lv_label_set_text(status_bar->edit_icon, LV_SYMBOL_EDIT);
    lv_obj_set_style_text_color(status_bar->edit_icon, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_bar->edit_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(status_bar->edit_icon, LV_ALIGN_LEFT_MID, UI_MARGIN_MEDIUM + 54, 0);
    
    status_bar->folder_icon = lv_label_create(status_bar->status_bar);
    lv_label_set_text(status_bar->folder_icon, LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_text_color(status_bar->folder_icon, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_bar->folder_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(status_bar->folder_icon, LV_ALIGN_LEFT_MID, UI_MARGIN_MEDIUM + 81, 0);
    
    return status_bar;
}

void ui_update_status_time(ui_status_bar_t *status_bar, const char *time_str)
{
    if (status_bar && status_bar->time_label && time_str) {
        lv_label_set_text(status_bar->time_label, time_str);
    }
}

void ui_destroy_status_bar(ui_status_bar_t *status_bar)
{
    if (status_bar) {
        if (status_bar->status_bar) {
            lv_obj_del(status_bar->status_bar);
        }
        free(status_bar);
    }
}

// =============================================================================
// BARRA DE NAVEGAÇÃO
// =============================================================================

ui_nav_bar_t *ui_create_nav_bar(lv_obj_t *parent, void (*nav_callback)(ui_screen_t screen))
{
    ui_nav_bar_t *nav_bar = malloc(sizeof(ui_nav_bar_t));
    if (!nav_bar) return NULL;
    
    nav_bar->current_screen = UI_SCREEN_HOME;
    
    // Container principal
    nav_bar->nav_bar = lv_obj_create(parent);
    lv_obj_set_size(nav_bar->nav_bar, UI_SCREEN_WIDTH, UI_NAV_BAR_HEIGHT);
    lv_obj_align(nav_bar->nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar->nav_bar, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(nav_bar->nav_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(nav_bar->nav_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(nav_bar->nav_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_side(nav_bar->nav_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nav_bar->nav_bar, UI_MARGIN_SMALL, LV_PART_MAIN);
    
    // IMPORTANTE: Desabilitar scroll na navbar
    lv_obj_clear_flag(nav_bar->nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // Configuração dos botões
    const char *nav_icons[] = {LV_SYMBOL_HOME, "S", "M", LV_SYMBOL_SETTINGS, LV_SYMBOL_LIST};
    const char *nav_labels[] = {"Home", "Sensors", "Medicao", "Config", "About"};
    
    for (int i = 0; i < UI_SCREEN_COUNT; i++) {
        // Container do botão
        lv_obj_t *btn_container = lv_obj_create(nav_bar->nav_bar);
        lv_obj_set_size(btn_container, (UI_SCREEN_WIDTH / UI_SCREEN_COUNT) - UI_MARGIN_MEDIUM, UI_NAV_BAR_HEIGHT - UI_MARGIN_MEDIUM);
        lv_obj_align(btn_container, LV_ALIGN_LEFT_MID, i * (UI_SCREEN_WIDTH / UI_SCREEN_COUNT) + UI_MARGIN_SMALL, 0);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_container, 0, LV_PART_MAIN);
        
        // Desabilitar scroll no container do botão
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Botão clicável
        nav_bar->nav_buttons[i] = lv_btn_create(btn_container);
        lv_obj_set_size(nav_bar->nav_buttons[i], (UI_SCREEN_WIDTH / UI_SCREEN_COUNT) - UI_MARGIN_LARGE, UI_NAV_BAR_HEIGHT - UI_MARGIN_LARGE);
        lv_obj_center(nav_bar->nav_buttons[i]);
        lv_obj_set_style_bg_opa(nav_bar->nav_buttons[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(nav_bar->nav_buttons[i], 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(nav_bar->nav_buttons[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(nav_bar->nav_buttons[i], 2, LV_PART_MAIN);
        
        // Container para organizar ícone e texto verticalmente
        lv_obj_t *content = lv_obj_create(nav_bar->nav_buttons[i]);
        lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_center(content);
        lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        // Ícone
        lv_obj_t *icon = lv_label_create(content);
        lv_label_set_text(icon, nav_icons[i]);
        lv_obj_set_style_text_color(icon, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_text_font(icon, UI_FONT_MEDIUM, LV_PART_MAIN);
        
        // Label
        lv_obj_t *label = lv_label_create(content);
        lv_label_set_text(label, nav_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
        
        // Callback
        lv_obj_set_user_data(nav_bar->nav_buttons[i], nav_callback);
        lv_obj_add_event_cb(nav_bar->nav_buttons[i], nav_button_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }
    
    return nav_bar;
}

void ui_update_nav_active(ui_nav_bar_t *nav_bar, ui_screen_t active_screen)
{
    if (!nav_bar) return;
    
    for (int i = 0; i < UI_SCREEN_COUNT; i++) {
        if (nav_bar->nav_buttons[i]) {
            if (i == active_screen) {
                lv_obj_set_style_bg_color(nav_bar->nav_buttons[i], lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
                lv_obj_set_style_bg_opa(nav_bar->nav_buttons[i], LV_OPA_20, LV_PART_MAIN);
            } else {
                lv_obj_set_style_bg_opa(nav_bar->nav_buttons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }
    }
    
    nav_bar->current_screen = active_screen;
}

void ui_destroy_nav_bar(ui_nav_bar_t *nav_bar)
{
    if (nav_bar) {
        if (nav_bar->nav_bar) {
            lv_obj_del(nav_bar->nav_bar);
        }
        free(nav_bar);
    }
}

// =============================================================================
// COMPONENTES GERAIS
// =============================================================================

lv_obj_t *ui_create_card(lv_obj_t *parent, int width, int height)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    ui_apply_card_style(card);
    return card;
}

lv_obj_t *ui_create_button_primary(lv_obj_t *parent, const char *text, lv_event_cb_t callback)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    ui_apply_button_primary_style(btn);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    if (callback) {
        lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

lv_obj_t *ui_create_button_secondary(lv_obj_t *parent, const char *text, lv_event_cb_t callback)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    ui_apply_button_secondary_style(btn);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    if (callback) {
        lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);
    }
    
    return btn;
}

lv_obj_t *ui_create_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, text);
    ui_apply_title_style(title);
    return title;
}

lv_obj_t *ui_create_body_text(lv_obj_t *parent, const char *text)
{
    lv_obj_t *body = lv_label_create(parent);
    lv_label_set_text(body, text);
    ui_apply_body_style(body);
    return body;
}

lv_obj_t *ui_create_content_area(lv_obj_t *parent)
{
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(content, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(content, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, UI_MARGIN_MEDIUM, LV_PART_MAIN);
    return content;
}