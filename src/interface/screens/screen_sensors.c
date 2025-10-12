/**
 * @file screen_sensors.c
 * @brief Implementação da tela de sensores
 * @author ItaloSixx
 * @date 2025
 */

#include "screen_sensors.h"
#include "../styles/ui_styles.h"
#include <stdio.h>

// Labels para dados dinâmicos
static lv_obj_t *bno_data_label = NULL;
static lv_obj_t *other_data_label = NULL;

lv_obj_t *screen_sensors_create(lv_obj_t *parent)
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
    lv_obj_t *title = ui_create_title(screen, "Sensor Monitoring");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_MARGIN_SMALL);
    
    // Card do BNO055
    lv_obj_t *bno_card = ui_create_card(screen, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 100);
    lv_obj_align(bno_card, LV_ALIGN_TOP_MID, 0, 40);
    
    // Título do BNO055
    lv_obj_t *bno_title = lv_label_create(bno_card);
    lv_label_set_text(bno_title, LV_SYMBOL_CHARGE " BNO055 - Orientation");
    lv_obj_set_style_text_color(bno_title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(bno_title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(bno_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Dados do BNO055
    bno_data_label = lv_label_create(bno_card);
    lv_label_set_text(bno_data_label, "Pitch: 0.0 deg\\nRoll: 0.0 deg\\nYaw: 0.0 deg\\nBattery: 0.0 V\\nStatus: Connecting...");
    lv_obj_set_style_text_color(bno_data_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(bno_data_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_align(bno_data_label, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // Card de outros sensores
    lv_obj_t *other_card = ui_create_card(screen, UI_SCREEN_WIDTH - (UI_MARGIN_MEDIUM * 2), 100);
    lv_obj_align(other_card, LV_ALIGN_TOP_MID, 0, 155);
    
    // Título outros sensores
    lv_obj_t *other_title = lv_label_create(other_card);
    lv_label_set_text(other_title, LV_SYMBOL_SETTINGS " Other Sensors");
    lv_obj_set_style_text_color(other_title, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(other_title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(other_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Dados outros sensores
    other_data_label = lv_label_create(other_card);
    lv_label_set_text(other_data_label, "LIDAR: -- mm\\nBattery: -- V\\nLow Battery: --\\nTimestamp: --\\nI2C: Active");
    lv_obj_set_style_text_color(other_data_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(other_data_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_align(other_data_label, LV_ALIGN_TOP_LEFT, 0, 25);
    
    return screen;
}

void screen_sensors_update(lv_obj_t *screen, const sensor_data_t *data)
{
    if (!screen || !data) return;
    
    // Atualizar dados do BNO055
    if (bno_data_label) {
        char bno_text[200];
        snprintf(bno_text, sizeof(bno_text), 
                "Pitch: %.1f deg\\nRoll: %.1f deg\\nYaw: %.1f deg\\nBattery: %.1f V\\nStatus: %s",
                data->pitch, data->roll, data->yaw, data->battery_voltage,
                data->bno055_valid ? "Connected" : "Disconnected");
        lv_label_set_text(bno_data_label, bno_text);
    }
    
    // Atualizar outros sensores
    if (other_data_label) {
        char other_text[200];
        snprintf(other_text, sizeof(other_text),
                "LIDAR: %d mm\\nBattery: %.2f V\\nLow Battery: %s\\nTimestamp: %lu\\nI2C: Active",
                data->lidar_distance, data->battery_voltage, 
                data->low_battery ? "Yes" : "No", data->timestamp);
        lv_label_set_text(other_data_label, other_text);
    }
}

void screen_sensors_destroy(lv_obj_t *screen)
{
    if (screen) {
        bno_data_label = NULL;
        other_data_label = NULL;
        lv_obj_del(screen);
    }
}