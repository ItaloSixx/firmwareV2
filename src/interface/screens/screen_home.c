/**
 * @file screen_home.c
 * @brief Implementação da tela home
 * @author ItaloSixx
 * @date 2025
 */

#include "screen_home.h"
#include "../styles/ui_styles.h"
#include "measurement/measurement_main.h"

// Callbacks dos botões
static void button_measurement_full_cb(lv_event_t *e);
static void button_measurement_single_cb(lv_event_t *e);
static void button_config_cb(lv_event_t *e);

// Callback de navegação global
static void (*g_navigation_callback)(ui_screen_t screen) = NULL;

lv_obj_t *screen_home_create(lv_obj_t *parent)
{
    // Container principal da tela
    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_set_size(screen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, UI_MARGIN_MEDIUM, LV_PART_MAIN);
    
    // Desabilitar scroll na tela home
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Card principal
    lv_obj_t *main_card = ui_create_card(screen, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 
                                        UI_CONTENT_HEIGHT - (UI_MARGIN_MEDIUM * 2));
    lv_obj_center(main_card);
    
    // Desabilitar scroll no card principal
    lv_obj_clear_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Título
    lv_obj_t *title = ui_create_title(main_card, "EcoDashboard");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_MARGIN_MEDIUM);
    
    // Botão de medição completa
    lv_obj_t *measure_full_btn = lv_btn_create(main_card);
    lv_obj_set_size(measure_full_btn, 320, 70);
    lv_obj_align(measure_full_btn, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(measure_full_btn, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_set_style_radius(measure_full_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(measure_full_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(measure_full_btn, LV_OPA_30, LV_PART_MAIN);
    
    lv_obj_t *measure_full_label = lv_label_create(measure_full_btn);
    lv_label_set_text(measure_full_label, LV_SYMBOL_EDIT " MEDICAO COMPLETA");
    lv_obj_set_style_text_color(measure_full_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(measure_full_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_center(measure_full_label);
    lv_obj_add_event_cb(measure_full_btn, button_measurement_full_cb, LV_EVENT_CLICKED, NULL);

    // Botão de medição única
    lv_obj_t *measure_single_btn = lv_btn_create(main_card);
    lv_obj_set_size(measure_single_btn, 320, 70);
    lv_obj_align(measure_single_btn, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_set_style_bg_color(measure_single_btn, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius(measure_single_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(measure_single_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(measure_single_btn, LV_OPA_20, LV_PART_MAIN);
    
    lv_obj_t *measure_single_label = lv_label_create(measure_single_btn);
    lv_label_set_text(measure_single_label, LV_SYMBOL_OK " MEDICAO UNICA");
    lv_obj_set_style_text_color(measure_single_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(measure_single_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_center(measure_single_label);
    lv_obj_add_event_cb(measure_single_btn, button_measurement_single_cb, LV_EVENT_CLICKED, NULL);

    // Barra de ação inferior com botão de Config mais largo
    lv_obj_t *button_bar = lv_obj_create(main_card);
    lv_obj_set_size(button_bar, 380, 70);
    lv_obj_align(button_bar, LV_ALIGN_TOP_MID, 0, 230);
    lv_obj_set_style_bg_opa(button_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button_bar, 0, LV_PART_MAIN);
    lv_obj_set_layout(button_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *config_btn = lv_btn_create(button_bar);
    lv_obj_set_size(config_btn, 320, 60);
    ui_apply_button_secondary_style(config_btn);
    lv_obj_add_event_cb(config_btn, button_config_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *config_content = lv_obj_create(config_btn);
    lv_obj_set_size(config_content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(config_content);
    lv_obj_set_style_bg_opa(config_content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(config_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(config_content, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(config_content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(config_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *config_icon = lv_label_create(config_content);
    lv_label_set_text(config_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(config_icon, UI_FONT_MEDIUM, LV_PART_MAIN);

    lv_obj_t *config_label = lv_label_create(config_content);
    lv_label_set_text(config_label, "CONFIG");
    lv_obj_set_style_text_font(config_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_pad_left(config_label, 8, LV_PART_MAIN);
    
    return screen;
}

void screen_home_update(lv_obj_t *screen)
{
    // Atualizar dados se necessário
    (void)screen;
}

void screen_home_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
}

// =============================================================================
// CALLBACKS DOS BOTÕES
// =============================================================================

void screen_home_set_navigation_callback(void (*callback)(ui_screen_t screen))
{
    g_navigation_callback = callback;
}

static void button_measurement_full_cb(lv_event_t *e)
{
    (void)e;
    measurement_set_mode(MEASUREMENT_MODE_FULL);
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_MEASUREMENT);
    }
}

static void button_measurement_single_cb(lv_event_t *e)
{
    (void)e;
    measurement_set_mode(MEASUREMENT_MODE_SINGLE);
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_MEASUREMENT);
    }
}

static void button_config_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_SETTINGS);
    }
}
