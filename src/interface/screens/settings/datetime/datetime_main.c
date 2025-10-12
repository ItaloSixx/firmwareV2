/**
 * @file datetime_main.c
 * @brief Implementação da tela principal de configurações de data e hora
 * @author ItaloSixx
 * @date 2025
 */

#include "datetime_main.h"
#include "../../../styles/ui_styles.h"
#include <esp_log.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "DATETIME_MAIN";

// Estado atual de data/hora
typedef struct {
    int hour;
    int minute;
    int day;
    int month;
    int year;
} datetime_state_t;

static datetime_state_t g_datetime = {
    .hour = 15,
    .minute = 36,
    .day = 10,
    .month = 4,
    .year = 2024
};

// Callbacks
static datetime_navigation_cb_t g_navigation_callback = NULL;
static datetime_back_cb_t g_back_callback = NULL;

// Labels para atualização
static lv_obj_t *g_time_value_label = NULL;
static lv_obj_t *g_date_value_label = NULL;

// Callbacks
static void time_settings_cb(lv_event_t *e);
static void date_settings_cb(lv_event_t *e);
static void back_button_cb(lv_event_t *e);

// Função auxiliar
static void update_display_labels(void);

lv_obj_t *datetime_main_create(lv_obj_t *parent)
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
    
    // Botão voltar
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 80, 35);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_center(back_label);
    
    // Título
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Date & Time");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // === OPÇÃO DE HORA ===
    lv_obj_t *time_container = lv_btn_create(screen);
    lv_obj_set_size(time_container, LV_PCT(100), 70);
    lv_obj_set_pos(time_container, 0, 70);
    lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(time_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(time_container, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(time_container, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(time_container, time_settings_cb, LV_EVENT_CLICKED, NULL);
    
    // Efeito de clique
    lv_obj_set_style_bg_opa(time_container, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(time_container, lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_PRESSED);
    lv_obj_set_style_radius(time_container, 8, LV_PART_MAIN);
    
    lv_obj_t *time_title = lv_label_create(time_container);
    lv_label_set_text(time_title, "Time");
    lv_obj_set_style_text_color(time_title, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_title, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(time_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    g_time_value_label = lv_label_create(time_container);
    lv_obj_set_style_text_color(g_time_value_label, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_time_value_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(g_time_value_label, LV_ALIGN_TOP_LEFT, 0, 25);
    
    lv_obj_t *time_arrow = lv_label_create(time_container);
    lv_label_set_text(time_arrow, ">");
    lv_obj_set_style_text_color(time_arrow, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_arrow, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(time_arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // === OPÇÃO DE DATA ===
    lv_obj_t *date_container = lv_btn_create(screen);
    lv_obj_set_size(date_container, LV_PCT(100), 70);
    lv_obj_set_pos(date_container, 0, 160);
    lv_obj_set_style_bg_opa(date_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(date_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(date_container, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(date_container, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(date_container, date_settings_cb, LV_EVENT_CLICKED, NULL);
    
    // Efeito de clique
    lv_obj_set_style_bg_opa(date_container, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(date_container, lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_PRESSED);
    lv_obj_set_style_radius(date_container, 8, LV_PART_MAIN);
    
    lv_obj_t *date_title = lv_label_create(date_container);
    lv_label_set_text(date_title, "Date");
    lv_obj_set_style_text_color(date_title, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(date_title, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(date_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    g_date_value_label = lv_label_create(date_container);
    lv_obj_set_style_text_color(g_date_value_label, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_date_value_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(g_date_value_label, LV_ALIGN_TOP_LEFT, 0, 25);
    
    lv_obj_t *date_arrow = lv_label_create(date_container);
    lv_label_set_text(date_arrow, ">");
    lv_obj_set_style_text_color(date_arrow, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(date_arrow, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(date_arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // Atualizar labels com valores atuais
    update_display_labels();
    
    ESP_LOGI(TAG, "DateTime main screen created successfully");
    return screen;
}

// =============================================================================
// CALLBACK IMPLEMENTATIONS
// =============================================================================

static void time_settings_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Time button clicked!");
    if (g_navigation_callback) {
        ESP_LOGI(TAG, "Calling navigation callback for time_picker");
        g_navigation_callback("time_picker");
    } else {
        ESP_LOGW(TAG, "Navigation callback is NULL!");
    }
}

static void date_settings_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Date button clicked!");
    if (g_navigation_callback) {
        ESP_LOGI(TAG, "Calling navigation callback for date_picker");
        g_navigation_callback("date_picker");
    } else {
        ESP_LOGW(TAG, "Navigation callback is NULL!");
    }
}

static void back_button_cb(lv_event_t *e)
{
    (void)e;
    if (g_back_callback) {
        g_back_callback();
    }
}

static void update_display_labels(void)
{
    if (g_time_value_label) {
        lv_label_set_text_fmt(g_time_value_label, "%02d:%02d", g_datetime.hour, g_datetime.minute);
    }
    
    if (g_date_value_label) {
        const char *months[] = {"January", "February", "March", "April", "May", "June", 
                               "July", "August", "September", "October", "November", "December"};
        lv_label_set_text_fmt(g_date_value_label, "%02d %s %04d", 
                             g_datetime.day, months[g_datetime.month - 1], g_datetime.year);
    }
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

void datetime_main_set_navigation_callback(datetime_navigation_cb_t callback)
{
    g_navigation_callback = callback;
}

void datetime_main_set_back_callback(datetime_back_cb_t callback)
{
    g_back_callback = callback;
}

void datetime_main_update(lv_obj_t *screen)
{
    (void)screen;
    update_display_labels();
}

void datetime_main_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
    g_time_value_label = NULL;
    g_date_value_label = NULL;
}

void datetime_get_current(int *hour, int *minute, int *day, int *month, int *year)
{
    if (hour) *hour = g_datetime.hour;
    if (minute) *minute = g_datetime.minute;
    if (day) *day = g_datetime.day;
    if (month) *month = g_datetime.month;
    if (year) *year = g_datetime.year;
}

void datetime_set_current(int hour, int minute, int day, int month, int year)
{
    g_datetime.hour = hour;
    g_datetime.minute = minute;
    g_datetime.day = day;
    g_datetime.month = month;
    g_datetime.year = year;
    
    // Atualizar o horário do sistema
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    
    time_t new_time = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = new_time, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    update_display_labels();
    
    ESP_LOGI(TAG, "DateTime set: %02d/%02d/%04d %02d:%02d", day, month, year, hour, minute);
}

void datetime_main_set_time(int hour, int minute)
{
    g_datetime.hour = hour;
    g_datetime.minute = minute;
    
    ESP_LOGI(TAG, "Setting time to: %02d:%02d (keeping date %02d/%02d/%04d)", hour, minute, g_datetime.day, g_datetime.month, g_datetime.year);
    
    // Atualizar o horário do sistema mantendo a data atual
    struct tm timeinfo = {0};
    timeinfo.tm_year = g_datetime.year - 1900;
    timeinfo.tm_mon = g_datetime.month - 1;
    timeinfo.tm_mday = g_datetime.day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    
    ESP_LOGI(TAG, "Before mktime: %04d-%02d-%02d %02d:%02d:%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    time_t new_time = mktime(&timeinfo);
    
    ESP_LOGI(TAG, "After mktime: %04d-%02d-%02d %02d:%02d:%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    struct timeval tv = { .tv_sec = new_time, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    update_display_labels();
    
    ESP_LOGI(TAG, "Time set completed");
}

void datetime_main_set_date(int day, int month, int year)
{
    g_datetime.day = day;
    g_datetime.month = month;
    g_datetime.year = year;
    
    // Atualizar a data do sistema mantendo a hora atual
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = g_datetime.hour;
    timeinfo.tm_min = g_datetime.minute;
    timeinfo.tm_sec = 0;
    
    time_t new_time = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = new_time, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    update_display_labels();
    
    ESP_LOGI(TAG, "Date set: %02d/%02d/%04d", day, month, year);
}