/**
 * @file time_picker.c
 * @brief Implementação do seletor de hora
 * @author ItaloSixx
 * @date 2025
 */

#include "time_picker.h"
#include "datetime_main.h"
#include "../../../styles/ui_styles.h"
#include <esp_log.h>
#include <stdio.h>

static const char *TAG = "TIME_PICKER";

// Callback de navegação
static time_picker_navigation_cb_t g_navigation_callback = NULL;

// Objetos dos rollers
static lv_obj_t *g_hour_roller = NULL;
static lv_obj_t *g_minute_roller = NULL;

// Callbacks
static void ok_button_cb(lv_event_t *e);
static void cancel_button_cb(lv_event_t *e);

lv_obj_t *time_picker_create(lv_obj_t *parent)
{
    // Container principal da tela
    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_set_size(screen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, UI_MARGIN_MEDIUM, LV_PART_MAIN);
    
    // Header com título e botão voltar
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    
    // Botão voltar (Cancel)
    lv_obj_t *cancel_btn = lv_btn_create(header);
    lv_obj_set_size(cancel_btn, 80, 35);
    lv_obj_align(cancel_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(cancel_btn, cancel_button_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    
    // Título
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Set Time");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Botão OK
    lv_obj_t *ok_btn = lv_btn_create(header);
    lv_obj_set_size(ok_btn, 60, 35);
    lv_obj_align(ok_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(ok_btn, ok_button_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "OK");
    lv_obj_center(ok_label);
    
    // Container dos seletores
    lv_obj_t *picker_container = lv_obj_create(screen);
    lv_obj_set_size(picker_container, LV_PCT(100), 200);
    lv_obj_set_pos(picker_container, 0, 80);
    lv_obj_set_style_bg_opa(picker_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(picker_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(picker_container, 20, LV_PART_MAIN);
    
    // Labels
    lv_obj_t *hour_label = lv_label_create(picker_container);
    lv_label_set_text(hour_label, "Hour");
    lv_obj_set_style_text_color(hour_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(hour_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(hour_label, 80, 10);
    
    lv_obj_t *minute_label = lv_label_create(picker_container);
    lv_label_set_text(minute_label, "Minute");
    lv_obj_set_style_text_color(minute_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(minute_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(minute_label, 280, 10);
    
    // Hour roller
    g_hour_roller = lv_roller_create(picker_container);
    lv_obj_set_size(g_hour_roller, 120, 140);
    lv_obj_set_pos(g_hour_roller, 50, 40);
    lv_roller_set_visible_row_count(g_hour_roller, 5);
    
    // Criar opções de horas (0-23)
    char hour_options[150] = "";
    for (int i = 0; i < 24; i++) {
        char temp[8];
        snprintf(temp, sizeof(temp), "%02d", i);
        strcat(hour_options, temp);
        if (i < 23) strcat(hour_options, "\n");
    }
    lv_roller_set_options(g_hour_roller, hour_options, LV_ROLLER_MODE_NORMAL);
    
    // Minute roller
    g_minute_roller = lv_roller_create(picker_container);
    lv_obj_set_size(g_minute_roller, 120, 140);
    lv_obj_set_pos(g_minute_roller, 250, 40);
    lv_roller_set_visible_row_count(g_minute_roller, 5);
    
    // Criar opções de minutos
    char minute_options[300] = "";
    for (int i = 0; i < 60; i++) {
        char temp[8];
        snprintf(temp, sizeof(temp), "%02d", i);
        strcat(minute_options, temp);
        if (i < 59) strcat(minute_options, "\n");
    }
    lv_roller_set_options(g_minute_roller, minute_options, LV_ROLLER_MODE_NORMAL);
    
    // Definir valores iniciais com base no datetime atual
    int current_hour, current_minute;
    datetime_get_current(&current_hour, &current_minute, NULL, NULL, NULL);
    lv_roller_set_selected(g_hour_roller, current_hour, LV_ANIM_OFF);
    lv_roller_set_selected(g_minute_roller, current_minute, LV_ANIM_OFF);
    
    ESP_LOGI(TAG, "Time picker screen created successfully");
    return screen;
}

// =============================================================================
// CALLBACK IMPLEMENTATIONS
// =============================================================================

static void ok_button_cb(lv_event_t *e)
{
    (void)e;
    
    if (g_hour_roller && g_minute_roller) {
        int selected_hour = lv_roller_get_selected(g_hour_roller);
        int selected_minute = lv_roller_get_selected(g_minute_roller);
        
        // Obter valores atuais de data
        int current_day, current_month, current_year;
        datetime_get_current(NULL, NULL, &current_day, &current_month, &current_year);
        
        // Atualizar apenas a hora
        datetime_set_current(selected_hour, selected_minute, current_day, current_month, current_year);
        
        ESP_LOGI(TAG, "Time updated: %02d:%02d", selected_hour, selected_minute);
    }
    
    if (g_navigation_callback) {
        g_navigation_callback("datetime_main");
    }
}

static void cancel_button_cb(lv_event_t *e)
{
    (void)e;
    if (g_navigation_callback) {
        g_navigation_callback("datetime_main");
    }
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

void time_picker_set_navigation_callback(time_picker_navigation_cb_t callback)
{
    g_navigation_callback = callback;
}

void time_picker_update(lv_obj_t *screen)
{
    (void)screen;
    // Atualizar se necessário
}

void time_picker_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
    g_hour_roller = NULL;
    g_minute_roller = NULL;
}

void time_picker_set_initial(int hour, int minute)
{
    if (g_hour_roller && hour >= 0 && hour < 24) {
        lv_roller_set_selected(g_hour_roller, hour, LV_ANIM_OFF);
    }
    
    if (g_minute_roller && minute >= 0 && minute < 60) {
        lv_roller_set_selected(g_minute_roller, minute, LV_ANIM_OFF);
    }
}