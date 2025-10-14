/**
 * @file ui_styles.c
 * @brief Implementação dos estilos visuais
 * @author ItaloSixx
 * @date 2025
 */

#include "ui_styles.h"

/**
 * @brief Aplica estilo de card moderno
 */
void ui_apply_card_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(obj, UI_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, UI_MARGIN_MEDIUM, LV_PART_MAIN);
}

/**
 * @brief Aplica estilo de botão primário
 */
void ui_apply_button_primary_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_PRIMARY_VARIANT), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius(obj, UI_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(obj, 4, LV_PART_MAIN);
}

/**
 * @brief Aplica estilo de botão secundário
 */
void ui_apply_button_secondary_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(obj, UI_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_10, LV_PART_MAIN);
}

/**
 * @brief Aplica estilo de texto título
 */
void ui_apply_title_style(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

/**
 * @brief Aplica estilo de texto corpo
 */
void ui_apply_body_style(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, UI_FONT_MEDIUM, LV_PART_MAIN);
}

/**
 * @brief Aplica estilo de container principal
 */
void ui_apply_container_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}