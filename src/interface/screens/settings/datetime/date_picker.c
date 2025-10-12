/**
 * @file date_picker.c
 * @brief Implementação do seletor de data
 * @author ItaloSixx
 * @date 2025
 */

#include "date_picker.h"
#include "datetime_main.h"
#include "../../../styles/ui_styles.h"
#include <esp_log.h>
#include <stdio.h>

static const char *TAG = "DATE_PICKER";

// Callback de navegação
static date_picker_navigation_cb_t g_navigation_callback = NULL;

// Objetos dos rollers
static lv_obj_t *g_day_roller = NULL;
static lv_obj_t *g_month_roller = NULL;
static lv_obj_t *g_year_roller = NULL;

// Callbacks
static void ok_button_cb(lv_event_t *e);
static void cancel_button_cb(lv_event_t *e);

lv_obj_t *date_picker_create(lv_obj_t *parent)
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
    lv_label_set_text(title, "Set Date");
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
    lv_obj_set_style_pad_all(picker_container, 10, LV_PART_MAIN);
    
    // Labels para os rollers
    lv_obj_t *day_label = lv_label_create(picker_container);
    lv_label_set_text(day_label, "Day");
    lv_obj_set_style_text_color(day_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(day_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(day_label, 50, 10);
    
    lv_obj_t *month_label = lv_label_create(picker_container);
    lv_label_set_text(month_label, "Month");
    lv_obj_set_style_text_color(month_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(month_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(month_label, 170, 10);
    
    lv_obj_t *year_label = lv_label_create(picker_container);
    lv_label_set_text(year_label, "Year");
    lv_obj_set_style_text_color(year_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(year_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(year_label, 310, 10);
    
    // Day roller
    g_day_roller = lv_roller_create(picker_container);
    lv_obj_set_size(g_day_roller, 90, 140);
    lv_obj_set_pos(g_day_roller, 30, 40);
    lv_roller_set_visible_row_count(g_day_roller, 5);
    
    char day_options[200] = "";
    for (int i = 1; i <= 31; i++) {
        char temp[8];
        snprintf(temp, sizeof(temp), "%02d", i);
        strcat(day_options, temp);
        if (i < 31) strcat(day_options, "\n");
    }
    lv_roller_set_options(g_day_roller, day_options, LV_ROLLER_MODE_NORMAL);
    
    // Month roller
    g_month_roller = lv_roller_create(picker_container);
    lv_obj_set_size(g_month_roller, 110, 140);
    lv_obj_set_pos(g_month_roller, 140, 40);
    lv_roller_set_visible_row_count(g_month_roller, 5);
    
    const char *month_options = "Jan\nFeb\nMar\nApr\nMay\nJun\nJul\nAug\nSep\nOct\nNov\nDec";
    lv_roller_set_options(g_month_roller, month_options, LV_ROLLER_MODE_NORMAL);
    
    // Year roller
    g_year_roller = lv_roller_create(picker_container);
    lv_obj_set_size(g_year_roller, 100, 140);
    lv_obj_set_pos(g_year_roller, 280, 40);
    lv_roller_set_visible_row_count(g_year_roller, 5);
    
    char year_options[300] = "";
    for (int i = 2020; i <= 2030; i++) {
        char temp[8];
        snprintf(temp, sizeof(temp), "%04d", i);
        strcat(year_options, temp);
        if (i < 2030) strcat(year_options, "\n");
    }
    lv_roller_set_options(g_year_roller, year_options, LV_ROLLER_MODE_NORMAL);
    
    // Definir valores iniciais com base no datetime atual
    int current_day, current_month, current_year;
    datetime_get_current(NULL, NULL, &current_day, &current_month, &current_year);
    lv_roller_set_selected(g_day_roller, current_day - 1, LV_ANIM_OFF);
    lv_roller_set_selected(g_month_roller, current_month - 1, LV_ANIM_OFF);
    lv_roller_set_selected(g_year_roller, current_year - 2020, LV_ANIM_OFF);
    
    ESP_LOGI(TAG, "Date picker screen created successfully");
    return screen;
}

// =============================================================================
// CALLBACK IMPLEMENTATIONS
// =============================================================================

static void ok_button_cb(lv_event_t *e)
{
    (void)e;
    
    if (g_day_roller && g_month_roller && g_year_roller) {
        int selected_day = lv_roller_get_selected(g_day_roller) + 1;
        int selected_month = lv_roller_get_selected(g_month_roller) + 1;
        int selected_year = 2020 + lv_roller_get_selected(g_year_roller);
        
        // Obter valores atuais de hora
        int current_hour, current_minute;
        datetime_get_current(&current_hour, &current_minute, NULL, NULL, NULL);
        
        // Atualizar apenas a data
        datetime_set_current(current_hour, current_minute, selected_day, selected_month, selected_year);
        
        ESP_LOGI(TAG, "Date updated: %02d/%02d/%04d", selected_day, selected_month, selected_year);
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

void date_picker_set_navigation_callback(date_picker_navigation_cb_t callback)
{
    g_navigation_callback = callback;
}

void date_picker_update(lv_obj_t *screen)
{
    (void)screen;
    // Atualizar se necessário
}

void date_picker_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
    g_day_roller = NULL;
    g_month_roller = NULL;
    g_year_roller = NULL;
}

void date_picker_set_initial(int day, int month, int year)
{
    if (g_day_roller && day >= 1 && day <= 31) {
        lv_roller_set_selected(g_day_roller, day - 1, LV_ANIM_OFF);
    }
    
    if (g_month_roller && month >= 1 && month <= 12) {
        lv_roller_set_selected(g_month_roller, month - 1, LV_ANIM_OFF);
    }
    
    if (g_year_roller && year >= 2020 && year <= 2030) {
        lv_roller_set_selected(g_year_roller, year - 2020, LV_ANIM_OFF);
    }
}