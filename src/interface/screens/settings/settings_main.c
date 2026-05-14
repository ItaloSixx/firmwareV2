#include "lvgl.h"
static void wifi_settings_cb(lv_event_t *e);
static void datetime_settings_cb(lv_event_t *e);
static void sync_time_ntp_cb(lv_event_t *e);
static lv_obj_t *g_datetime_modal = NULL;
static lv_obj_t *g_hour_roller = NULL;
static lv_obj_t *g_minute_roller = NULL;
static lv_obj_t *g_day_roller = NULL;
static lv_obj_t *g_month_roller = NULL;
static lv_obj_t *g_year_roller = NULL;
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
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "wifi/wifi_main.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
    y_pos += 60;
    
    // === DATA E HORA ===
    lv_obj_t *datetime_container = lv_obj_create(scroll_container);
    lv_obj_set_size(datetime_container, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 50);
    lv_obj_set_pos(datetime_container, 0, y_pos);
    lv_obj_set_style_bg_opa(datetime_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(datetime_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(datetime_container, 10, LV_PART_MAIN);
    lv_obj_add_flag(datetime_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(datetime_container, datetime_settings_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *datetime_label = lv_label_create(datetime_container);
    lv_label_set_text(datetime_label, LV_SYMBOL_SETTINGS " Data e Hora");
    lv_obj_set_style_text_color(datetime_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(datetime_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(datetime_label, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t *datetime_arrow = lv_label_create(datetime_container);
    lv_label_set_text(datetime_arrow, "> ");
    lv_obj_set_style_text_color(datetime_arrow, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(datetime_arrow, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(datetime_arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    
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

// === DATETIME MODAL CALLBACKS ===
static void datetime_modal_close_cb(lv_event_t *e)
{
    (void)e;
    if (g_datetime_modal) {
        lv_obj_del(g_datetime_modal);
        g_datetime_modal = NULL;
    }
}

static void datetime_modal_save_cb(lv_event_t *e)
{
    (void)e;
    
    // Obter valores dos rollers
    int hour = lv_roller_get_selected(g_hour_roller);
    int minute = lv_roller_get_selected(g_minute_roller);
    int day = lv_roller_get_selected(g_day_roller) + 1;
    int month = lv_roller_get_selected(g_month_roller);
    int year = lv_roller_get_selected(g_year_roller) + 2024;
    
    // Configurar struct tm
    struct tm timeinfo = {
        .tm_sec = 0,
        .tm_min = minute,
        .tm_hour = hour,
        .tm_mday = day,
        .tm_mon = month,
        .tm_year = year - 1900,
        .tm_isdst = -1
    };
    
    // Converter para time_t e definir hora do sistema
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    ESP_LOGI(TAG, "Data/Hora configurada: %02d/%02d/%04d %02d:%02d", 
             day, month + 1, year, hour, minute);
    
    // Fechar modal
    if (g_datetime_modal) {
        lv_obj_del(g_datetime_modal);
        g_datetime_modal = NULL;
    }
}

static void sync_time_ntp_cb(lv_event_t *e)
{
    (void)e;
    
    if (!wifi_main_is_connected()) {
        ESP_LOGW(TAG, "Wi-Fi nao conectado, impossivel sincronizar NTP");
        return;
    }
    
    ESP_LOGI(TAG, "Iniciando sincronizacao NTP...");
    
    // Parar SNTP se já estiver rodando
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
        vTaskDelay(pdMS_TO_TICKS(100)); // Aguardar parar completamente
    }
    
    // Configurar SNTP com timezone Brasil (UTC-3)
    setenv("TZ", "BRT3", 1);
    tzset();
    
    // Configurar SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_sntp_init();
    
    // Aguardar sincronização com timeout de 10 segundos
    int retry = 0;
    const int retry_count = 20;
    time_t now = 0;
    struct tm timeinfo = {0};
    
    while (timeinfo.tm_year < (2024 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Aguardando NTP sync... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    
    if (timeinfo.tm_year >= (2024 - 1900)) {
        ESP_LOGI(TAG, "NTP sync OK: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGW(TAG, "NTP sync timeout - hora pode nao estar correta");
    }
    
    // Fechar modal após sincronização
    if (g_datetime_modal) {
        lv_obj_del(g_datetime_modal);
        g_datetime_modal = NULL;
    }
}

static void datetime_settings_cb(lv_event_t *e)
{
    (void)e;
    
    // Criar modal - tamanho otimizado
    g_datetime_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_datetime_modal, 280, 290);
    lv_obj_center(g_datetime_modal);
    lv_obj_set_style_bg_color(g_datetime_modal, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(g_datetime_modal, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_datetime_modal, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_datetime_modal, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_datetime_modal, 10, LV_PART_MAIN);
    lv_obj_clear_flag(g_datetime_modal, LV_OBJ_FLAG_SCROLLABLE);
    
    // Título
    lv_obj_t *title = lv_label_create(g_datetime_modal);
    lv_label_set_text(title, "Data e Hora");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Obter hora atual
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Mostrar hora/data atual
    char current_dt[48];
    strftime(current_dt, sizeof(current_dt), "%d/%m/%Y %H:%M", &timeinfo);
    lv_obj_t *current_dt_label = lv_label_create(g_datetime_modal);
    lv_label_set_text_fmt(current_dt_label, "Atual: %s", current_dt);
    lv_obj_set_style_text_color(current_dt_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(current_dt_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_align(current_dt_label, LV_ALIGN_TOP_MID, 0, 20);

    // === LINHA 1: HORA (HH : MM) ===
    lv_obj_t *time_label = lv_label_create(g_datetime_modal);
    lv_label_set_text(time_label, "Hora:");
    lv_obj_set_style_text_color(time_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_pos(time_label, 10, 40);
    
    // Roller de horas
    g_hour_roller = lv_roller_create(g_datetime_modal);
    lv_roller_set_options(g_hour_roller, 
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_hour_roller, 2);
    lv_obj_set_size(g_hour_roller, 50, 45);
    lv_obj_set_pos(g_hour_roller, 60, 38);
    lv_roller_set_selected(g_hour_roller, timeinfo.tm_hour, LV_ANIM_OFF);
    
    lv_obj_t *colon = lv_label_create(g_datetime_modal);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(colon, 115, 52);
    
    // Roller de minutos
    g_minute_roller = lv_roller_create(g_datetime_modal);
    char minutes[180] = "";
    for (int i = 0; i < 60; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d%s", i, i < 59 ? "\n" : "");
        strcat(minutes, buf);
    }
    lv_roller_set_options(g_minute_roller, minutes, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_minute_roller, 2);
    lv_obj_set_size(g_minute_roller, 50, 45);
    lv_obj_set_pos(g_minute_roller, 130, 38);
    lv_roller_set_selected(g_minute_roller, timeinfo.tm_min, LV_ANIM_OFF);

    // === LINHA 2: DATA (DD / MM / YYYY) ===
    lv_obj_t *date_label = lv_label_create(g_datetime_modal);
    lv_label_set_text(date_label, "Data:");
    lv_obj_set_style_text_color(date_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_pos(date_label, 10, 95);
    
    // Roller de dia
    g_day_roller = lv_roller_create(g_datetime_modal);
    char days[124] = "";
    for (int i = 1; i <= 31; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d%s", i, i < 31 ? "\n" : "");
        strcat(days, buf);
    }
    lv_roller_set_options(g_day_roller, days, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_day_roller, 2);
    lv_obj_set_size(g_day_roller, 45, 45);
    lv_obj_set_pos(g_day_roller, 60, 93);
    lv_roller_set_selected(g_day_roller, timeinfo.tm_mday - 1, LV_ANIM_OFF);
    
    lv_obj_t *slash1 = lv_label_create(g_datetime_modal);
    lv_label_set_text(slash1, "/");
    lv_obj_set_style_text_font(slash1, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(slash1, 108, 107);
    
    // Roller de mês
    g_month_roller = lv_roller_create(g_datetime_modal);
    lv_roller_set_options(g_month_roller, 
        "Jan\nFev\nMar\nAbr\nMai\nJun\nJul\nAgo\nSet\nOut\nNov\nDez",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_month_roller, 2);
    lv_obj_set_size(g_month_roller, 45, 45);
    lv_obj_set_pos(g_month_roller, 120, 93);
    lv_roller_set_selected(g_month_roller, timeinfo.tm_mon, LV_ANIM_OFF);
    
    lv_obj_t *slash2 = lv_label_create(g_datetime_modal);
    lv_label_set_text(slash2, "/");
    lv_obj_set_style_text_font(slash2, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_pos(slash2, 168, 107);
    
    // Roller de ano
    g_year_roller = lv_roller_create(g_datetime_modal);
    lv_roller_set_options(g_year_roller, "2024\n2025\n2026\n2027\n2028\n2029\n2030", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_year_roller, 2);
    lv_obj_set_size(g_year_roller, 55, 45);
    lv_obj_set_pos(g_year_roller, 180, 93);
    int year_idx = timeinfo.tm_year + 1900 - 2024;
    if (year_idx < 0) year_idx = 0;
    if (year_idx > 6) year_idx = 6;
    lv_roller_set_selected(g_year_roller, year_idx, LV_ANIM_OFF);
    
    // === LINHA 3: BOTÕES ===
    // Botão Sincronizar NTP
    lv_obj_t *ntp_btn = lv_btn_create(g_datetime_modal);
    lv_obj_set_size(ntp_btn, 75, 40);
    lv_obj_set_pos(ntp_btn, 10, 150);
    lv_obj_set_style_bg_color(ntp_btn, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
    lv_obj_add_event_cb(ntp_btn, sync_time_ntp_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *ntp_label = lv_label_create(ntp_btn);
    lv_label_set_text(ntp_label, "NTP");
    lv_obj_set_style_text_color(ntp_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ntp_label);
    
    // Botão Cancelar
    lv_obj_t *cancel_btn = lv_btn_create(g_datetime_modal);
    lv_obj_set_size(cancel_btn, 75, 40);
    lv_obj_set_pos(cancel_btn, 95, 150);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
    lv_obj_add_event_cb(cancel_btn, datetime_modal_close_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "X");
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(cancel_label);
    
    // Botão Salvar
    lv_obj_t *save_btn = lv_btn_create(g_datetime_modal);
    lv_obj_set_size(save_btn, 75, 40);
    lv_obj_set_pos(save_btn, 180, 150);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_add_event_cb(save_btn, datetime_modal_save_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "OK");
    lv_obj_set_style_text_color(save_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(save_label);
    
    ESP_LOGI(TAG, "Datetime settings modal opened");
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