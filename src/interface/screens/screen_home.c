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
static void button_system_cb(lv_event_t *e);
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
    
    // Card principal
    lv_obj_t *main_card = ui_create_card(screen, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 
                                        UI_CONTENT_HEIGHT - (UI_MARGIN_MEDIUM * 2));
    lv_obj_center(main_card);
    
    // Título
    lv_obj_t *title = ui_create_title(main_card, "System Dashboard");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_MARGIN_MEDIUM);
    
    // Grid para botões 2x2
    lv_obj_t *button_grid = lv_obj_create(main_card);
    lv_obj_set_size(button_grid, 300, 150);
    lv_obj_center(button_grid);
    lv_obj_set_style_bg_opa(button_grid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button_grid, UI_MARGIN_SMALL, LV_PART_MAIN);
    lv_obj_set_layout(button_grid, LV_LAYOUT_GRID);
    
    // Configuração do grid 2x2
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(button_grid, col_dsc, row_dsc);
    
    // Textos dos botões
    const char* button_texts[] = {
        "SENSORS",
        "SYSTEM", 
        "CONFIG",
        "ABOUT"
    };
    
    // Callbacks dos botões
    lv_event_cb_t button_callbacks[] = {
        button_sensors_cb,
        button_system_cb,
        button_config_cb,
        button_about_cb
    };
    
    // Criar botões 2x2
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(button_grid);
        lv_obj_set_size(btn, 120, 60);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, i % 2, 1, 
                             LV_GRID_ALIGN_CENTER, i / 2, 1);
        
        // Estilo do botão
        ui_apply_button_primary_style(btn);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_PART_MAIN);
        
        // Label do botão
        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, button_texts[i]);
        lv_obj_center(btn_label);
        lv_obj_set_style_text_font(btn_label, UI_FONT_MEDIUM, LV_PART_MAIN);
        
        // Callback
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, NULL);
    }
    
    // Texto inferior
    lv_obj_t *bottom_text = ui_create_body_text(main_card, "What do you want to do today?");
    lv_obj_align(bottom_text, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN_MEDIUM);
    
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

static void button_system_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback(UI_SCREEN_SENSORS); // Por enquanto vai para sensores
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