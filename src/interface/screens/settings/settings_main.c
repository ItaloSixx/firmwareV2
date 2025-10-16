#include "lvgl.h"
static void wifi_settings_cb(lv_event_t *e);
/**
 * @file settings_main.c
 * @brief Implementação da tela principal de configurações
 * @author ItaloSixx
 * @date 2025
 */

#include "settings_main.h"
#include "../../styles/ui_styles.h"
#include "../../../display.h"
#include <esp_log.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "SETTINGS_MAIN";

// Estado das configurações
typedef struct {
    uint8_t brightness;
    bool night_mode;
    bool wifi_enabled;
    bool bluetooth_enabled;
} settings_state_t;

static settings_state_t g_settings = {
    .brightness = 80,
    .night_mode = false,
    .wifi_enabled = false,
    .bluetooth_enabled = false
};

// Callback de navegação
static settings_navigation_cb_t g_navigation_callback = NULL;

// Callbacks dos controles
static void brightness_slider_cb(lv_event_t *e);
static void night_mode_switch_cb(lv_event_t *e);
static void bluetooth_switch_cb(lv_event_t *e);

lv_obj_t *settings_main_create(lv_obj_t *parent)
{
    // Container principal da tela
    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_set_size(screen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, UI_MARGIN_MEDIUM, LV_PART_MAIN);
    
    // Container scrollável
    lv_obj_t *scroll_container = lv_obj_create(screen);
    lv_obj_set_size(scroll_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(scroll_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroll_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scroll_container, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER);
    
    // Título principal
    lv_obj_t *title = ui_create_title(scroll_container, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    int y_pos = 40;
    
    // === BRIGHTNESS CONTROL ===
    lv_obj_t *brightness_label = lv_label_create(scroll_container);
    lv_label_set_text_fmt(brightness_label, "Brightness: %d%%", g_settings.brightness);
    lv_obj_set_style_text_color(brightness_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(brightness_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(brightness_label, 0, y_pos);
    
    lv_obj_t *brightness_slider = lv_slider_create(scroll_container);
    lv_obj_set_size(brightness_slider, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 4), 20);
    lv_obj_set_pos(brightness_slider, 0, y_pos + 25);
    lv_slider_set_range(brightness_slider, 10, 100);
    lv_slider_set_value(brightness_slider, g_settings.brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, brightness_label);
    y_pos += 70;
    

    
    // === NIGHT MODE ===
    lv_obj_t *night_container = lv_obj_create(scroll_container);
    lv_obj_set_size(night_container, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 50);
    lv_obj_set_pos(night_container, 0, y_pos);
    lv_obj_set_style_bg_opa(night_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(night_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(night_container, 10, LV_PART_MAIN);
    
    lv_obj_t *night_label = lv_label_create(night_container);
    lv_label_set_text(night_label, "Night Mode");
    lv_obj_set_style_text_color(night_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(night_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(night_label, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t *night_switch = lv_switch_create(night_container);
    lv_obj_align(night_switch, LV_ALIGN_RIGHT_MID, -10, 0);
    if (g_settings.night_mode) {
        lv_obj_add_state(night_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(night_switch, night_mode_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    y_pos += 60;
    
    // === WI-FI ===
    lv_obj_t *wifi_container = lv_obj_create(scroll_container);
    lv_obj_set_size(wifi_container, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 50);
    lv_obj_set_pos(wifi_container, 0, y_pos);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_container, 10, LV_PART_MAIN);
        lv_obj_add_flag(wifi_container, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(wifi_container, wifi_settings_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *wifi_label = lv_label_create(wifi_container);
    lv_label_set_text(wifi_label, "Wi-Fi Settings");
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(wifi_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Ícone de seta para indicar navegação
    lv_obj_t *wifi_arrow = lv_label_create(wifi_container);
    lv_label_set_text(wifi_arrow, "> ");
    lv_obj_set_style_text_color(wifi_arrow, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_arrow, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(wifi_arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    y_pos += 60;
    
    // === BLUETOOTH ===
    lv_obj_t *bt_container = lv_obj_create(scroll_container);
    lv_obj_set_size(bt_container, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 50);
    lv_obj_set_pos(bt_container, 0, y_pos);
    lv_obj_set_style_bg_opa(bt_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bt_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bt_container, 10, LV_PART_MAIN);
    
    lv_obj_t *bt_label = lv_label_create(bt_container);
    lv_label_set_text(bt_label, "Bluetooth");
    lv_obj_set_style_text_color(bt_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(bt_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(bt_label, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t *bt_switch = lv_switch_create(bt_container);
    lv_obj_align(bt_switch, LV_ALIGN_RIGHT_MID, -10, 0);
    if (g_settings.bluetooth_enabled) {
        lv_obj_add_state(bt_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(bt_switch, bluetooth_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    ESP_LOGI(TAG, "Settings main screen created successfully");
    return screen;
}

// =============================================================================
// CALLBACK IMPLEMENTATIONS
// =============================================================================

static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    
    g_settings.brightness = (uint8_t)lv_slider_get_value(slider);
    lv_label_set_text_fmt(label, "Brightness: %d%%", g_settings.brightness);
    
    // Controle real do brilho da tela
    esp_err_t ret = bsp_display_brightness_set(g_settings.brightness);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Brightness set to: %d%%", g_settings.brightness);
    } else {
        ESP_LOGE(TAG, "Failed to set brightness: %d", ret);
    }
}

static void night_mode_switch_cb(lv_event_t *e)
{
    lv_obj_t *switch_obj = lv_event_get_target(e);
    g_settings.night_mode = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
    
    ESP_LOGI(TAG, "Night mode: %s", g_settings.night_mode ? "ON" : "OFF");
    
    // TODO: Implementar mudança de tema para modo noturno
    if (g_settings.night_mode) {
        // Aplicar tema escuro
    } else {
        // Aplicar tema claro
    }
}



static void bluetooth_switch_cb(lv_event_t *e)
{
    lv_obj_t *switch_obj = lv_event_get_target(e);
    g_settings.bluetooth_enabled = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
    
    ESP_LOGI(TAG, "Bluetooth: %s", g_settings.bluetooth_enabled ? "ENABLED" : "DISABLED");
    
    // TODO: Implementar controle real do Bluetooth
    if (g_settings.bluetooth_enabled) {
        // Inicializar Bluetooth
    } else {
        // Desativar Bluetooth
    }
}



static void wifi_settings_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback("wifi_main");
    }
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

void settings_main_set_navigation_callback(settings_navigation_cb_t callback)
{
    g_navigation_callback = callback;
}

void settings_main_update(lv_obj_t *screen)
{
    // Atualizar configurações se necessário
    (void)screen;
}

void settings_main_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
}

// Funções para gerenciar configurações externamente
uint8_t settings_main_get_brightness(void) { return g_settings.brightness; }
bool settings_main_get_night_mode(void) { return g_settings.night_mode; }
bool settings_main_get_wifi_enabled(void) { return g_settings.wifi_enabled; }
bool settings_main_get_bluetooth_enabled(void) { return g_settings.bluetooth_enabled; }

void settings_main_set_brightness(uint8_t value) { 
    g_settings.brightness = value; 
    bsp_display_brightness_set(value);
}
void settings_main_set_night_mode(bool enabled) { g_settings.night_mode = enabled; }
void settings_main_set_wifi_enabled(bool enabled) { g_settings.wifi_enabled = enabled; }
void settings_main_set_bluetooth_enabled(bool enabled) { g_settings.bluetooth_enabled = enabled; }