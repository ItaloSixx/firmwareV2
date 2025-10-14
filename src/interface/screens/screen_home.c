/**
 * @file screen_home.c
 * @brief Implementação da tela home
 * @author ItaloSixx
 * @date 2025
 */

#include "screen_home.h"
#include "../styles/ui_styles.h"

// Callbacks dos botões
static void button_sensors_cb(lv_event_t *e);
static void button_measurement_cb(lv_event_t *e);
static void button_config_cb(lv_event_t *e);
static void button_about_cb(lv_event_t *e);

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
    lv_obj_t *title = ui_create_title(main_card, "System Dashboard");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_MARGIN_MEDIUM);
    
    // Botão principal de medição (destacado)
    lv_obj_t *measure_btn = lv_btn_create(main_card);
    lv_obj_set_size(measure_btn, 320, 70);
    lv_obj_align(measure_btn, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(measure_btn, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_set_style_radius(measure_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(measure_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(measure_btn, LV_OPA_30, LV_PART_MAIN);
    
    lv_obj_t *measure_label = lv_label_create(measure_btn);
    lv_label_set_text(measure_label, LV_SYMBOL_EDIT " MEDICAO DE PLANTAS");
    lv_obj_set_style_text_color(measure_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(measure_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_center(measure_label);
    lv_obj_add_event_cb(measure_btn, button_measurement_cb, LV_EVENT_CLICKED, NULL);

    // Grid para botões secundários 3x1
    lv_obj_t *button_grid = lv_obj_create(main_card);
    lv_obj_set_size(button_grid, 350, 60);
    lv_obj_align(button_grid, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_opa(button_grid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button_grid, UI_MARGIN_SMALL, LV_PART_MAIN);
    lv_obj_set_layout(button_grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Textos dos botões secundários
    const char* button_texts[] = {
        "SENSORES",
        "CONFIG", 
        "SOBRE"
    };
    
    // Ícones dos botões
    const char* button_icons[] = {
        LV_SYMBOL_EYE_OPEN,
        LV_SYMBOL_SETTINGS,
        LV_SYMBOL_LIST
    };
    
    // Callbacks dos botões
    lv_event_cb_t button_callbacks[] = {
        button_sensors_cb,
        button_config_cb,
        button_about_cb
    };
    
    // Criar botões secundários
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(button_grid);
        lv_obj_set_size(btn, 100, 60);
        ui_apply_button_secondary_style(btn);
        
        // Container para ícone e texto
        lv_obj_t *content = lv_obj_create(btn);
        lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_center(content);
        lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        // Ícone
        lv_obj_t *icon = lv_label_create(content);
        lv_label_set_text(icon, button_icons[i]);
        lv_obj_set_style_text_font(icon, UI_FONT_MEDIUM, LV_PART_MAIN);
        
        // Label do botão
        lv_obj_t *btn_label = lv_label_create(content);
        lv_label_set_text(btn_label, button_texts[i]);
        lv_obj_set_style_text_font(btn_label, UI_FONT_SMALL, LV_PART_MAIN);
        
        // Callback
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, NULL);
    }
    
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

static void button_sensors_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_SENSORS);
    }
}

static void button_measurement_cb(lv_event_t *e)
{
    (void)e;
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

static void button_about_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_ABOUT);
    }
}