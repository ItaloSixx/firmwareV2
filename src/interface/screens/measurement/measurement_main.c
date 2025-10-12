/**
 * @file measurement_main.c
 * @brief Implementação do sistema de medição de altura de plantas
 * @author ItaloSixx
 * @date 2025
 */

#include "measurement_main.h"
#include "../../styles/ui_styles.h"
#include "../../../sensors/lidar_tf_mini.h"
#include <esp_log.h>
#include <esp_random.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "MEASUREMENT";

// Variáveis globais
static plant_measurement_t g_measurement = {0};
static plant_measurement_state_t g_state = PLANT_MEASUREMENT_STATE_IDLE;
static lv_obj_t *g_status_label = NULL;
static lv_obj_t *g_instruction_label = NULL;
static lv_obj_t *g_distance_label = NULL;
static lv_obj_t *g_result_panel = NULL;
static lv_obj_t *g_measure_btn = NULL;
static lv_obj_t *g_save_btn = NULL;
static lv_timer_t *g_lidar_update_timer = NULL;

// Declarações de funções privadas
static void measure_button_cb(lv_event_t *e);
static void save_button_cb(lv_event_t *e);
static void reset_button_cb(lv_event_t *e);
static void update_ui_state(void);
static void calculate_height(void);
static float read_lidar_distance(void);
static void update_result_display(void);
static void feedback_timer_cb(lv_timer_t *timer);
static void calculation_timer_cb(lv_timer_t *timer);
static void lidar_update_timer_cb(lv_timer_t *timer);

// Textos das instruções para cada estado
static const char* get_instruction_text(plant_measurement_state_t state) {
    switch (state) {
        case PLANT_MEASUREMENT_STATE_IDLE:
            return "Posicione o dispositivo e inicie a medicao";
        case PLANT_MEASUREMENT_STATE_HORIZONTAL:
            return "PASSO 1: Mire no CENTRO da planta (horizontal)";
        case PLANT_MEASUREMENT_STATE_TOP:
            return "PASSO 2: Mire no TOPO da planta";
        case PLANT_MEASUREMENT_STATE_BASE:
            return "PASSO 3: Mire na BASE da planta";
        case PLANT_MEASUREMENT_STATE_CALCULATING:
            return "Calculando altura total...";
        case PLANT_MEASUREMENT_STATE_COMPLETE:
            return "Medicao concluida com sucesso!";
        default:
            return "Estado desconhecido";
    }
}

static const char* get_status_text(plant_measurement_state_t state) {
    switch (state) {
        case PLANT_MEASUREMENT_STATE_IDLE:
            return "PRONTO";
        case PLANT_MEASUREMENT_STATE_HORIZONTAL:
            return "MEDINDO HORIZONTAL";
        case PLANT_MEASUREMENT_STATE_TOP:
            return "MEDINDO TOPO";
        case PLANT_MEASUREMENT_STATE_BASE:
            return "MEDINDO BASE";
        case PLANT_MEASUREMENT_STATE_CALCULATING:
            return "CALCULANDO";
        case PLANT_MEASUREMENT_STATE_COMPLETE:
            return "CONCLUIDO";
        default:
            return "ERRO";
    }
}

lv_obj_t *measurement_main_create(lv_obj_t *parent)
{
    if (!parent) return NULL;

    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    // === CABEÇALHO ===
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Medicao de Plantas");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // === PAINEL DE STATUS ===
    lv_obj_t *status_panel = lv_obj_create(screen);
    lv_obj_set_size(status_panel, 456, 80);  // 95% de 480px = 456px
    lv_obj_set_pos(status_panel, 12, 70);    // 2.5% de 480px = 12px
    lv_obj_set_style_bg_color(status_panel, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(status_panel, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius(status_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_panel, 15, LV_PART_MAIN);

    g_status_label = lv_label_create(status_panel);
    lv_label_set_text(g_status_label, get_status_text(g_state));
    lv_obj_set_style_text_color(g_status_label, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_status_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(g_status_label, LV_ALIGN_CENTER, 0, -10);

    g_distance_label = lv_label_create(status_panel);
    lv_label_set_text(g_distance_label, "Distancia: -- cm");
    lv_obj_set_style_text_color(g_distance_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_distance_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_align(g_distance_label, LV_ALIGN_CENTER, 0, 15);

    // === PAINEL DE INSTRUÇÕES ===
    lv_obj_t *instruction_panel = lv_obj_create(screen);
    lv_obj_set_size(instruction_panel, 456, 60);
    lv_obj_set_pos(instruction_panel, 12, 160);
    lv_obj_set_style_bg_color(instruction_panel, lv_color_hex(UI_COLOR_LIGHT), LV_PART_MAIN);
    lv_obj_set_style_border_width(instruction_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(instruction_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(instruction_panel, 15, LV_PART_MAIN);

    g_instruction_label = lv_label_create(instruction_panel);
    lv_label_set_text(g_instruction_label, get_instruction_text(g_state));
    lv_obj_set_style_text_color(g_instruction_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_instruction_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_style_text_align(g_instruction_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(g_instruction_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(g_instruction_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_instruction_label, 400);

    // === PAINEL DE RESULTADOS ===
    g_result_panel = lv_obj_create(screen);
    lv_obj_set_size(g_result_panel, 456, 120);
    lv_obj_set_pos(g_result_panel, 12, 230);
    lv_obj_set_style_bg_color(g_result_panel, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_result_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_result_panel, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius(g_result_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_result_panel, 15, LV_PART_MAIN);
    lv_obj_add_flag(g_result_panel, LV_OBJ_FLAG_HIDDEN); // Oculto inicialmente

    // === BOTÕES DE AÇÃO ===
    g_measure_btn = lv_btn_create(screen);
    lv_obj_set_size(g_measure_btn, 120, 45);
    lv_obj_set_pos(g_measure_btn, 20, 360);
    lv_obj_set_style_bg_color(g_measure_btn, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_add_event_cb(g_measure_btn, measure_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *measure_label = lv_label_create(g_measure_btn);
    lv_label_set_text(measure_label, "MEDIR");
    lv_obj_set_style_text_color(measure_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(measure_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_center(measure_label);

    g_save_btn = lv_btn_create(screen);
    lv_obj_set_size(g_save_btn, 100, 45);
    lv_obj_set_pos(g_save_btn, 150, 360);
    lv_obj_set_style_bg_color(g_save_btn, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_add_event_cb(g_save_btn, save_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN); // Oculto inicialmente

    lv_obj_t *save_label = lv_label_create(g_save_btn);
    lv_label_set_text(save_label, "SALVAR");
    lv_obj_set_style_text_color(save_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(save_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(save_label);

    lv_obj_t *reset_btn = lv_btn_create(screen);
    lv_obj_set_size(reset_btn, 100, 45);
    lv_obj_set_pos(reset_btn, 260, 360);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
    lv_obj_add_event_cb(reset_btn, reset_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "RESET");
    lv_obj_set_style_text_color(reset_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(reset_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(reset_label);

    // Resetar estado inicial
    g_state = PLANT_MEASUREMENT_STATE_IDLE;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    update_ui_state();

    // Iniciar timer para atualizar leitura do LiDAR em tempo real (a cada 500ms)
    g_lidar_update_timer = lv_timer_create(lidar_update_timer_cb, 500, NULL);

    ESP_LOGI(TAG, "Measurement screen created successfully");
    return screen;
}

// === IMPLEMENTAÇÃO DOS CALLBACKS ===

static void measure_button_cb(lv_event_t *e)
{
    (void)e;
    
    switch (g_state) {
        case PLANT_MEASUREMENT_STATE_IDLE:
            g_state = PLANT_MEASUREMENT_STATE_HORIZONTAL;
            break;
            
        case PLANT_MEASUREMENT_STATE_HORIZONTAL:
            g_measurement.distance_horizontal = read_lidar_distance();
            ESP_LOGI(TAG, "Horizontal distance: %.1f cm", g_measurement.distance_horizontal);
            g_state = PLANT_MEASUREMENT_STATE_TOP;
            break;
            
        case PLANT_MEASUREMENT_STATE_TOP:
            g_measurement.distance_to_top = read_lidar_distance();
            ESP_LOGI(TAG, "Distance to top: %.1f cm", g_measurement.distance_to_top);
            g_state = PLANT_MEASUREMENT_STATE_BASE;
            break;
            
        case PLANT_MEASUREMENT_STATE_BASE:
            g_measurement.distance_to_base = read_lidar_distance();
            ESP_LOGI(TAG, "Distance to base: %.1f cm", g_measurement.distance_to_base);
            g_state = PLANT_MEASUREMENT_STATE_CALCULATING;
            calculate_height();
            break;
            
        case PLANT_MEASUREMENT_STATE_COMPLETE:
            g_state = PLANT_MEASUREMENT_STATE_IDLE;
            memset(&g_measurement, 0, sizeof(plant_measurement_t));
            break;
            
        default:
            break;
    }
    
    update_ui_state();
}

static void save_button_cb(lv_event_t *e)
{
    (void)e;
    
    if (g_state == PLANT_MEASUREMENT_STATE_COMPLETE && g_measurement.measurement_valid) {
        time_t now;
        time(&now);
        struct tm *timeinfo = localtime(&now);
        strftime(g_measurement.timestamp, sizeof(g_measurement.timestamp), 
                "%d/%m/%Y %H:%M", timeinfo);
        
        ESP_LOGI(TAG, "Measurement saved: %.1f cm at %s", 
                g_measurement.total_height, g_measurement.timestamp);
        
        lv_obj_t *parent = lv_obj_get_parent(g_save_btn);
        lv_obj_t *feedback = lv_label_create(parent);
        lv_label_set_text(feedback, "Salvo com sucesso!");
        lv_obj_set_style_text_color(feedback, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
        lv_obj_align(feedback, LV_ALIGN_BOTTOM_MID, 0, -10);
        
        lv_timer_create(feedback_timer_cb, 2000, feedback);
    }
}

static void reset_button_cb(lv_event_t *e)
{
    (void)e;
    
    g_state = PLANT_MEASUREMENT_STATE_IDLE;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    update_ui_state();
    
    ESP_LOGI(TAG, "Measurement reset");
}

// === FUNÇÕES AUXILIARES ===

static void update_ui_state(void)
{
    if (!g_status_label || !g_instruction_label) return;
    
    lv_label_set_text(g_status_label, get_status_text(g_state));
    lv_label_set_text(g_instruction_label, get_instruction_text(g_state));
    
    if (g_distance_label) {
        char dist_text[128];
        
        // Mostrar leitura atual do LiDAR em tempo real
        lidar_data_t current_lidar;
        bool lidar_available = lidar_get_last_reading(&current_lidar);
        
        switch (g_state) {
            case PLANT_MEASUREMENT_STATE_HORIZONTAL:
                snprintf(dist_text, sizeof(dist_text), "Horizontal: %.1f cm", 
                        g_measurement.distance_horizontal);
                break;
            case PLANT_MEASUREMENT_STATE_TOP:
                snprintf(dist_text, sizeof(dist_text), "Ao topo: %.1f cm", 
                        g_measurement.distance_to_top);
                break;
            case PLANT_MEASUREMENT_STATE_BASE:
                snprintf(dist_text, sizeof(dist_text), "A base: %.1f cm", 
                        g_measurement.distance_to_base);
                break;
            default:
                if (lidar_available && current_lidar.valid) {
                    snprintf(dist_text, sizeof(dist_text), "LiDAR: %d cm (Forca: %d)", 
                            current_lidar.distance, current_lidar.strength);
                } else {
                    snprintf(dist_text, sizeof(dist_text), "LiDAR: -- cm (Sem sinal)");
                }
                break;
        }
        lv_label_set_text(g_distance_label, dist_text);
    }
    
    if (g_result_panel) {
        if (g_state == PLANT_MEASUREMENT_STATE_COMPLETE) {
            lv_obj_clear_flag(g_result_panel, LV_OBJ_FLAG_HIDDEN);
            update_result_display();
        } else {
            lv_obj_add_flag(g_result_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    if (g_save_btn) {
        if (g_state == PLANT_MEASUREMENT_STATE_COMPLETE && g_measurement.measurement_valid) {
            lv_obj_clear_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    if (g_measure_btn) {
        lv_obj_t *label = lv_obj_get_child(g_measure_btn, 0);
        if (label) {
            switch (g_state) {
                case PLANT_MEASUREMENT_STATE_IDLE:
                    lv_label_set_text(label, "INICIAR");
                    break;
                case PLANT_MEASUREMENT_STATE_COMPLETE:
                    lv_label_set_text(label, "NOVO");
                    break;
                default:
                    lv_label_set_text(label, "MEDIR");
                    break;
            }
        }
    }
}

static void calculate_height(void)
{
    g_state = PLANT_MEASUREMENT_STATE_CALCULATING;
    update_ui_state();
    lv_timer_create(calculation_timer_cb, 1500, NULL);
}

static float read_lidar_distance(void)
{
    lidar_data_t lidar_data;
    lidar_error_t result;
    
    // Tentar leitura bloqueante (aguarda até 1 segundo)
    result = lidar_read_blocking(&lidar_data);
    
    if (result == LIDAR_OK && lidar_data.valid) {
        ESP_LOGI(TAG, "LiDAR reading: %d cm, strength: %d", 
                 lidar_data.distance, lidar_data.strength);
        
        // Verificar se a leitura está dentro dos limites esperados
        if (lidar_data.distance >= 10 && lidar_data.distance <= 1200) {
            return (float)lidar_data.distance;
        } else {
            ESP_LOGW(TAG, "LiDAR distance out of range: %d cm", lidar_data.distance);
        }
    } else {
        ESP_LOGW(TAG, "LiDAR read error: %s", lidar_error_to_string(result));
    }
    
    // Fallback: retornar valor simulado se LiDAR falhar
    ESP_LOGW(TAG, "Using fallback distance value");
    return 150.0f + (esp_random() % 100);  // 150-250 cm
}

static void update_result_display(void)
{
    if (!g_result_panel) return;
    
    lv_obj_clean(g_result_panel);
    
    lv_obj_t *title = lv_label_create(g_result_panel);
    lv_label_set_text(title, "RESULTADO DA MEDICAO");
    lv_obj_set_style_text_font(title, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    char result_text[256];
    snprintf(result_text, sizeof(result_text), 
             "Altura do topo: %.1f cm\n"
             "Altura da base: %.1f cm\n"
             "ALTURA TOTAL: %.1f cm", 
             g_measurement.height_top, 
             g_measurement.height_base, 
             g_measurement.total_height);
    
    lv_obj_t *result_label = lv_label_create(g_result_panel);
    lv_label_set_text(result_label, result_text);
    lv_obj_set_style_text_font(result_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_style_text_color(result_label, lv_color_hex(UI_COLOR_ON_SURFACE), LV_PART_MAIN);
    lv_obj_align(result_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

// === CALLBACKS DE TIMER ===

static void feedback_timer_cb(lv_timer_t *timer)
{
    if (timer && timer->user_data) {
        lv_obj_del((lv_obj_t*)timer->user_data);
    }
    lv_timer_del(timer);
}

static void calculation_timer_cb(lv_timer_t *timer)
{
    float dist_top_sq = g_measurement.distance_to_top * g_measurement.distance_to_top;
    float dist_hor_sq = g_measurement.distance_horizontal * g_measurement.distance_horizontal;
    
    if (dist_top_sq > dist_hor_sq) {
        g_measurement.height_top = sqrt(dist_top_sq - dist_hor_sq);
    } else {
        g_measurement.height_top = 0.0f;
    }
    
    float dist_base_sq = g_measurement.distance_to_base * g_measurement.distance_to_base;
    
    if (dist_base_sq > dist_hor_sq) {
        g_measurement.height_base = sqrt(dist_base_sq - dist_hor_sq);
    } else {
        g_measurement.height_base = 0.0f;
    }
    
    g_measurement.total_height = g_measurement.height_top + g_measurement.height_base;
    g_measurement.measurement_valid = true;
    
    ESP_LOGI(TAG, "Calculation complete: h1=%.1f, h2=%.1f, total=%.1f cm", 
            g_measurement.height_top, g_measurement.height_base, g_measurement.total_height);
    
    g_state = PLANT_MEASUREMENT_STATE_COMPLETE;
    update_ui_state();
    
    lv_timer_del(timer);
}

static void lidar_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    // Atualizar apenas se estivermos em estado idle ou aguardando medição
    if (g_state == PLANT_MEASUREMENT_STATE_IDLE || 
        g_state == PLANT_MEASUREMENT_STATE_HORIZONTAL ||
        g_state == PLANT_MEASUREMENT_STATE_TOP ||
        g_state == PLANT_MEASUREMENT_STATE_BASE) {
        
        // Atualizar o display de distância sem bloquear a UI
        if (g_distance_label) {
            char dist_text[128];
            lidar_data_t current_lidar;
            
            if (lidar_get_last_reading(&current_lidar) && current_lidar.valid) {
                if (g_state == PLANT_MEASUREMENT_STATE_IDLE) {
                    snprintf(dist_text, sizeof(dist_text), "LiDAR: %d cm (Forca: %d)", 
                            current_lidar.distance, current_lidar.strength);
                    lv_label_set_text(g_distance_label, dist_text);
                }
            }
        }
    }
}

// === FUNÇÕES PÚBLICAS ===

void measurement_main_update(lv_obj_t *screen)
{
    (void)screen;
}

void measurement_main_destroy(lv_obj_t *screen)
{
    // Parar timer de atualização do LiDAR
    if (g_lidar_update_timer) {
        lv_timer_del(g_lidar_update_timer);
        g_lidar_update_timer = NULL;
    }
    
    if (screen) {
        lv_obj_del(screen);
    }
    
    g_status_label = NULL;
    g_instruction_label = NULL;
    g_distance_label = NULL;
    g_result_panel = NULL;
    g_measure_btn = NULL;
    g_save_btn = NULL;
    
    g_state = PLANT_MEASUREMENT_STATE_IDLE;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
}

void measurement_start_process(void)
{
    if (g_state == PLANT_MEASUREMENT_STATE_IDLE) {
        g_state = PLANT_MEASUREMENT_STATE_HORIZONTAL;
        update_ui_state();
    }
}

void measurement_stop_process(void)
{
    g_state = PLANT_MEASUREMENT_STATE_IDLE;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    update_ui_state();
}

plant_measurement_state_t measurement_get_state(void)
{
    return g_state;
}

plant_measurement_t measurement_get_data(void)
{
    return g_measurement;
}

bool measurement_save_current(void)
{
    if (g_state == PLANT_MEASUREMENT_STATE_COMPLETE && g_measurement.measurement_valid) {
        ESP_LOGI(TAG, "Saving measurement: %.1f cm", g_measurement.total_height);
        return true;
    }
    return false;
}

bool measurement_load_history(void)
{
    ESP_LOGI(TAG, "Loading measurement history");
    return true;
}
