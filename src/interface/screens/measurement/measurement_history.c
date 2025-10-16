#include "measurement_history.h"
#include "../../styles/ui_styles.h"
#include "../../../storage/sd_storage.h"
#include "../../../config.h"

#include <esp_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_MAX_LINE_LEN 256

typedef struct {
    char timestamp[32];
    float distance_horizontal;
    float distance_top;
    float distance_base;
    float height_top;
    float height_base;
    float height_total;
} measurement_history_entry_t;

static const char *TAG = "MEAS_HISTORY";
static lv_obj_t *s_history_screen = NULL;
static measurement_history_close_cb_t s_close_cb = NULL;
static bool s_close_triggered = false;

static void measurement_history_trigger_close(void)
{
    if (!s_close_triggered && s_close_cb) {
        s_close_triggered = true;
        s_close_cb();
    }
}

static void measurement_history_delete_event_cb(lv_event_t *e)
{
    (void)e;
    s_history_screen = NULL;
    measurement_history_trigger_close();
}

static void measurement_history_back_event_cb(lv_event_t *e)
{
    (void)e;
    if (s_history_screen) {
        lv_obj_del_async(s_history_screen);
    }
}

static void measurement_history_show_empty(lv_obj_t *parent, const char *message)
{
    if (!parent) {
        return;
    }

    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

static bool measurement_history_load_entries(measurement_history_entry_t **out_entries, size_t *out_count)
{
    if (!out_entries || !out_count) {
        return false;
    }

    *out_entries = NULL;
    *out_count = 0;

    if (!sd_storage_is_mounted()) {
        ESP_LOGW(TAG, "SD card not mounted, history unavailable");
        return false;
    }

    const char *csv_path = SD_MOUNT_POINT "/medicoes.csv";
    FILE *file = fopen(csv_path, "r");
    if (!file) {
        ESP_LOGW(TAG, "History file not found: %s", csv_path);
        return false;
    }

    size_t capacity = 16;
    measurement_history_entry_t *entries = malloc(capacity * sizeof(measurement_history_entry_t));
    if (!entries) {
        fclose(file);
        ESP_LOGE(TAG, "Failed to allocate memory for history entries");
        return false;
    }

    char line[HISTORY_MAX_LINE_LEN];
    bool header_skipped = false;
    size_t count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (!header_skipped) {
            header_skipped = true;
            continue; // skip header line
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) {
            continue;
        }

        if (count == capacity) {
            capacity *= 2;
            measurement_history_entry_t *tmp = realloc(entries, capacity * sizeof(measurement_history_entry_t));
            if (!tmp) {
                ESP_LOGE(TAG, "Failed to expand history entries array");
                free(entries);
                fclose(file);
                return false;
            }
            entries = tmp;
        }

        measurement_history_entry_t *entry = &entries[count];
        memset(entry, 0, sizeof(*entry));

        char *token = strtok(line, ",");
        int field = 0;
        while (token) {
            switch (field) {
                case 0:
                    strncpy(entry->timestamp, token, sizeof(entry->timestamp) - 1);
                    entry->timestamp[sizeof(entry->timestamp) - 1] = '\0';
                    break;
                case 1:
                    entry->distance_horizontal = strtof(token, NULL);
                    break;
                case 2:
                    entry->distance_top = strtof(token, NULL);
                    break;
                case 3:
                    entry->distance_base = strtof(token, NULL);
                    break;
                case 4:
                    entry->height_top = strtof(token, NULL);
                    break;
                case 5:
                    entry->height_base = strtof(token, NULL);
                    break;
                case 6:
                    entry->height_total = strtof(token, NULL);
                    break;
                default:
                    break;
            }
            token = strtok(NULL, ",");
            field++;
        }

        if (field >= 7) {
            count++;
        }
    }

    fclose(file);

    if (count == 0) {
        free(entries);
        return false;
    }

    *out_entries = entries;
    *out_count = count;
    return true;
}

static void measurement_history_populate(lv_obj_t *container)
{
    if (!container) {
        return;
    }

    measurement_history_entry_t *entries = NULL;
    size_t count = 0;

    if (!measurement_history_load_entries(&entries, &count)) {
        measurement_history_show_empty(container, sd_storage_is_mounted() ?
                                       "Nenhuma medicao encontrada no cartao SD" :
                                       "Cartao SD nao montado");
        return;
    }

    for (size_t i = count; i > 0; i--) {
        measurement_history_entry_t *entry = &entries[i - 1];

        lv_obj_t *card = lv_obj_create(container);
    lv_obj_set_width(card, LV_PCT(100));
        lv_obj_set_style_bg_color(card, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(card, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(card, 10, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(card);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));
        lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);

        char text[256];
        snprintf(text, sizeof(text), "%s\nHorizontal: %.1f cm  Topo: %.1f cm  Base: %.1f cm\nAltura Topo: %.1f cm  Altura Base: %.1f cm  Total: %.1f cm",
                 entry->timestamp,
                 entry->distance_horizontal,
                 entry->distance_top,
                 entry->distance_base,
                 entry->height_top,
                 entry->height_base,
                 entry->height_total);
        lv_label_set_text(label, text);
    }

    free(entries);
}

lv_obj_t *measurement_history_create(lv_obj_t *parent, measurement_history_close_cb_t close_cb)
{
    if (!parent) {
        return NULL;
    }

    if (s_history_screen) {
        lv_obj_del(s_history_screen);
        s_history_screen = NULL;
    }

    s_close_cb = close_cb;
    s_close_triggered = false;

    s_history_screen = lv_obj_create(parent);
    lv_obj_set_size(s_history_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_history_screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_history_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_history_screen, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_history_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_history_screen, measurement_history_delete_event_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *body = lv_obj_create(s_history_screen);
    lv_obj_set_size(body, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(body, 0, 60);
    lv_obj_set_style_bg_color(body, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_border_width(body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(body, 14, LV_PART_MAIN);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *header = lv_obj_create(s_history_screen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_move_foreground(header);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Historico de Medicoes");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 50, 0);

    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 42, 36);
    lv_obj_set_style_radius(back_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(back_btn, measurement_history_back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_center(back_label);

    measurement_history_populate(body);

    return s_history_screen;
}
