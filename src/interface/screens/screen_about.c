/**
 * @file screen_about.c
 * @brief Implementação da tela sobre
 * @author ItaloSixx
 * @date 2025
 */

#include "screen_about.h"
#include "../styles/ui_styles.h"

lv_obj_t *screen_about_create(lv_obj_t *parent)
{
    // Container principal da tela
    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_set_size(screen, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, UI_MARGIN_MEDIUM, LV_PART_MAIN);
    
    // Título principal
    lv_obj_t *title = ui_create_title(screen, "About System");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_MARGIN_SMALL);
    
    // Card de informações do hardware
    lv_obj_t *info_card = ui_create_card(screen, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 180);
    lv_obj_align(info_card, LV_ALIGN_TOP_MID, 0, 40);
    
    // Título informações do hardware
    lv_obj_t *info_title = lv_label_create(info_card);
    lv_label_set_text(info_title, "Hardware Information");
    lv_obj_set_style_text_color(info_title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Lista de informações
    lv_obj_t *info_list = lv_obj_create(info_card);
    lv_obj_set_size(info_list, LV_PCT(100), 150);
    lv_obj_align(info_list, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_obj_set_style_bg_opa(info_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_list, 0, LV_PART_MAIN);
    
    const char *info_items[] = {
        "Device: JC3248W535EN",
        "MCU: ESP32-S3 (240MHz)",
        "Display: 3.5\" 480x320 TFT",
        "Touch: Capacitive Touch",
        "Memory: 512KB SRAM",
        "Flash: 16MB",
        "Sensors: BNO055, BME280",
        "Connectivity: Wi-Fi, Bluetooth",
        "Version: v2.0.0",
        "Build: 2025-10-11"
    };
    
    for (int i = 0; i < 10; i++) {
        lv_obj_t *item = lv_label_create(info_list);
        lv_label_set_text(item, info_items[i]);
        lv_obj_set_style_text_color(item, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_text_font(item, UI_FONT_SMALL, LV_PART_MAIN);
        lv_obj_set_pos(item, 0, i * 14);
    }
    
    return screen;
}

void screen_about_update(lv_obj_t *screen)
{
    // Atualizar informações se necessário
    (void)screen;
}

void screen_about_destroy(lv_obj_t *screen)
{
    if (screen) {
        lv_obj_del(screen);
    }
}