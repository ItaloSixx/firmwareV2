#include "measurement_history.h"
#include "../../styles/ui_styles.h"
#include "../../../storage/sd_storage.h"
#include "../../../supabase/supabase_client.h"
#include "../../../config.h"

#include <esp_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_MAX_LINE_LEN 256

typedef struct {
    char timestamp[32];
    float distance_horizontal;
    float distance_top;
    float distance_base;
    float height_top;
    float height_base;
    float height_total;
    size_t line_number; // Para identificar a linha no arquivo CSV
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

// Callback para limpar memória alocada quando o card é deletado
static void card_delete_cb(lv_event_t *e)
{
    measurement_history_entry_t *entry = (measurement_history_entry_t *)lv_event_get_user_data(e);
    if (entry) {
        free(entry);
    }
}

// Função auxiliar para excluir uma linha do CSV
static bool delete_csv_line(size_t line_to_delete)
{
    const char *csv_path = SD_MOUNT_POINT "/medicoes.csv";
    const char *temp_path = SD_MOUNT_POINT "/medicoes_temp.csv";
    
    FILE *input = fopen(csv_path, "r");
    if (!input) {
        ESP_LOGE(TAG, "Failed to open CSV file for reading");
        return false;
    }
    
    FILE *output = fopen(temp_path, "w");
    if (!output) {
        ESP_LOGE(TAG, "Failed to create temporary file");
        fclose(input);
        return false;
    }
    
    char line[HISTORY_MAX_LINE_LEN];
    size_t current_line = 0;
    
    while (fgets(line, sizeof(line), input)) {
        current_line++;
        if (current_line != line_to_delete) {
            fputs(line, output);
        }
    }
    
    fclose(input);
    fclose(output);
    
    // Remover arquivo original e renomear temporário
    if (remove(csv_path) != 0) {
        ESP_LOGE(TAG, "Failed to remove original CSV file");
        remove(temp_path);
        return false;
    }
    
    if (rename(temp_path, csv_path) != 0) {
        ESP_LOGE(TAG, "Failed to rename temporary file");
        return false;
    }
    
    return true;
}

// Callback para excluir uma medição
static void delete_measurement_cb(lv_event_t *e)
{
    measurement_history_entry_t *entry = (measurement_history_entry_t *)lv_event_get_user_data(e);
    if (!entry) {
        ESP_LOGE(TAG, "Invalid entry pointer");
        return;
    }

    ESP_LOGI(TAG, "Deleting measurement at line %zu: %s", entry->line_number, entry->timestamp);
    
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *actions_container = lv_obj_get_parent(btn);
    lv_obj_t *card = lv_obj_get_parent(actions_container);
    
    if (delete_csv_line(entry->line_number)) {
        ESP_LOGI(TAG, "Measurement deleted successfully");
        
        // Animar e remover o card
        lv_obj_t *feedback = lv_label_create(card);
        lv_label_set_text(feedback, LV_SYMBOL_OK " Excluído!");
        lv_obj_set_style_text_color(feedback, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        lv_obj_set_style_text_font(feedback, UI_FONT_MEDIUM, LV_PART_MAIN);
        lv_obj_align(feedback, LV_ALIGN_CENTER, 0, 0);
        
        // Remover o card após 1 segundo
        lv_obj_del_delayed(card, 1000);
    } else {
        ESP_LOGE(TAG, "Failed to delete measurement");
        
        lv_obj_t *feedback = lv_label_create(card);
        lv_label_set_text(feedback, LV_SYMBOL_CLOSE " Erro ao excluir");
        lv_obj_set_style_text_color(feedback, lv_color_hex(0xFF5252), LV_PART_MAIN);
        lv_obj_set_style_text_font(feedback, UI_FONT_SMALL, LV_PART_MAIN);
        lv_obj_align(feedback, LV_ALIGN_CENTER, 0, 0);
    }
}

// Callback para sincronizar uma medição com Supabase
static void sync_measurement_cb(lv_event_t *e)
{
    measurement_history_entry_t *entry = (measurement_history_entry_t *)lv_event_get_user_data(e);
    if (!entry) {
        ESP_LOGE(TAG, "Invalid entry pointer");
        return;
    }

    ESP_LOGI(TAG, "Syncing measurement to Supabase: %s", entry->timestamp);
    
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *actions_container = lv_obj_get_parent(btn);
    lv_obj_t *card = lv_obj_get_parent(actions_container);
    
    if (!supabase_is_ready()) {
        // Criar feedback de erro
        lv_obj_t *feedback = lv_label_create(card);
        lv_label_set_text(feedback, "Servidor offline");
        lv_obj_set_style_text_color(feedback, lv_color_hex(0xFFA500), LV_PART_MAIN);
        lv_obj_set_style_text_font(feedback, UI_FONT_SMALL, LV_PART_MAIN);
        lv_obj_align(feedback, LV_ALIGN_CENTER, 0, 0);
        lv_obj_del_delayed(feedback, 2000);
        ESP_LOGW(TAG, "Supabase not ready");
        return;
    }
    
    // Criar feedback de "enviando"
    lv_obj_t *feedback = lv_label_create(card);
    lv_label_set_text(feedback, "Enviando...");
    lv_obj_set_style_text_color(feedback, lv_color_hex(0x1E90FF), LV_PART_MAIN);
    lv_obj_set_style_text_font(feedback, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_align(feedback, LV_ALIGN_CENTER, 0, 0);
    
    // Converter timestamp para formato ISO8601
    struct tm timeinfo = {0};
    int day, month, year, hour, min;
    
    ESP_LOGI(TAG, "Parsing timestamp: '%s'", entry->timestamp);
    
    // Parse timestamp no formato "dd/mm/yyyy HH:MM"
    if (sscanf(entry->timestamp, "%d/%d/%d %d:%d", 
               &day, &month, &year, &hour, &min) == 5) {
        
        ESP_LOGI(TAG, "Parsed: day=%d, month=%d, year=%d, hour=%d, min=%d", 
                 day, month, year, hour, min);
        
        timeinfo.tm_mday = day;
        timeinfo.tm_mon = month - 1;  // Mês é 0-based
        timeinfo.tm_year = year - 1900;  // Ano é desde 1900
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = min;
        timeinfo.tm_sec = 0;
        timeinfo.tm_isdst = -1;
        
        // Normalizar a estrutura tm
        mktime(&timeinfo);
        
        // Converter para ISO8601 UTC
        char iso_timestamp[32];
        strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
        
        ESP_LOGI(TAG, "ISO8601 timestamp: %s", iso_timestamp);
        
        // Preparar dados para envio
        supabase_measurement_t supabase_data = {
            .horizontal_cm = entry->distance_horizontal,
            .top_cm = entry->distance_top,
            .base_cm = entry->distance_base,
            .height_top_cm = entry->height_top,
            .height_base_cm = entry->height_base,
            .total_cm = entry->height_total
        };
        strncpy(supabase_data.timestamp, iso_timestamp, sizeof(supabase_data.timestamp) - 1);
        
        // Enviar para Supabase
        esp_err_t err = supabase_send_measurement(&supabase_data);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Measurement queued for sync successfully");
            lv_label_set_text(feedback, LV_SYMBOL_OK " Enviado!");
            lv_obj_set_style_text_color(feedback, lv_color_hex(0x4CAF50), LV_PART_MAIN);
            lv_obj_del_delayed(feedback, 2000);
        } else {
            ESP_LOGW(TAG, "Failed to queue measurement for sync");
            lv_label_set_text(feedback, LV_SYMBOL_CLOSE " Erro");
            lv_obj_set_style_text_color(feedback, lv_color_hex(0xFF5252), LV_PART_MAIN);
            lv_obj_del_delayed(feedback, 2000);
        }
    } else {
        ESP_LOGE(TAG, "Failed to parse timestamp: %s", entry->timestamp);
        lv_label_set_text(feedback, "Erro de formato");
        lv_obj_set_style_text_color(feedback, lv_color_hex(0xFF5252), LV_PART_MAIN);
        lv_obj_del_delayed(feedback, 2000);
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
    size_t line_number = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
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
        entry->line_number = line_number;

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
        // Alocar memória para a entrada que será passada como user_data
        measurement_history_entry_t *entry_copy = malloc(sizeof(measurement_history_entry_t));
        if (!entry_copy) {
            ESP_LOGE(TAG, "Failed to allocate memory for entry copy");
            continue;
        }
        memcpy(entry_copy, &entries[i - 1], sizeof(measurement_history_entry_t));
        measurement_history_entry_t *entry = entry_copy;

        lv_obj_t *card = lv_obj_create(container);
        lv_obj_set_width(card, LV_PCT(100));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(card, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        
        // Adicionar callback de DELETE para limpar memória
        lv_obj_add_event_cb(card, card_delete_cb, LV_EVENT_DELETE, entry);

        // Container de informações
        lv_obj_t *info_container = lv_obj_create(card);
        lv_obj_set_width(info_container, LV_PCT(100));
        lv_obj_set_height(info_container, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(info_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(info_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(info_container, 0, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(info_container);
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

        // Container de botões de ação
        lv_obj_t *actions_container = lv_obj_create(card);
        lv_obj_set_width(actions_container, LV_PCT(100));
        lv_obj_set_height(actions_container, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(actions_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(actions_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(actions_container, 4, LV_PART_MAIN);
        lv_obj_set_flex_flow(actions_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(actions_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Botão de sincronizar
        lv_obj_t *sync_btn = lv_btn_create(actions_container);
        lv_obj_set_size(sync_btn, 50, 36);
        lv_obj_set_style_radius(sync_btn, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(sync_btn, lv_color_hex(0x1E90FF), LV_PART_MAIN);
        lv_obj_add_event_cb(sync_btn, sync_measurement_cb, LV_EVENT_CLICKED, entry);

        lv_obj_t *sync_icon = lv_label_create(sync_btn);
        lv_label_set_text(sync_icon, LV_SYMBOL_UPLOAD);
        lv_obj_set_style_text_color(sync_icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(sync_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
        lv_obj_center(sync_icon);

        // Botão de excluir
        lv_obj_t *delete_btn = lv_btn_create(actions_container);
        lv_obj_set_size(delete_btn, 50, 36);
        lv_obj_set_style_radius(delete_btn, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(delete_btn, lv_color_hex(0xFF5252), LV_PART_MAIN);
        lv_obj_add_event_cb(delete_btn, delete_measurement_cb, LV_EVENT_CLICKED, entry);

        lv_obj_t *delete_icon = lv_label_create(delete_btn);
        lv_label_set_text(delete_icon, LV_SYMBOL_TRASH);
        lv_obj_set_style_text_color(delete_icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(delete_icon, UI_FONT_MEDIUM, LV_PART_MAIN);
        lv_obj_center(delete_icon);
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
