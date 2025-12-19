/**
 * @file measurement_main.c
 * @brief Implementação do sistema de medição de altura de plantas
 * @author ItaloSixx
 * @date 2025
 */

#include "measurement_main.h"
#include "measurement_history.h"
#include "../../styles/ui_styles.h"
#include "../../../sensors/lidar_tf_mini.h"
#include "../../../config.h"
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "../../../storage/sd_storage.h"
#include "../../../supabase/supabase_client.h"

static const char *TAG = "MEASUREMENT";

// Variáveis globais
static plant_measurement_t g_measurement = {0};
static plant_measurement_state_t g_state = PLANT_MEASUREMENT_STATE_IDLE;
static lv_obj_t *g_status_label = NULL;
static lv_obj_t *g_instruction_label = NULL;
static lv_obj_t *g_distance_label = NULL;
static lv_obj_t *g_result_panel = NULL;
static lv_obj_t *g_save_btn = NULL;
static lv_obj_t *g_cancel_btn = NULL;
static lv_obj_t *g_history_btn = NULL;
static lv_timer_t *g_lidar_update_timer = NULL;
static lv_obj_t *g_measurement_screen = NULL;
static lv_obj_t *g_history_subscreen = NULL;
static measurement_mode_t g_mode = MEASUREMENT_MODE_FULL;

// Widgets dinâmicos para cada medição
static lv_obj_t *g_widget_horizontal = NULL;
static lv_obj_t *g_widget_top = NULL;
static lv_obj_t *g_widget_base = NULL;
static lv_obj_t *g_widget_height1 = NULL;
static lv_obj_t *g_widget_height2 = NULL;
static lv_obj_t *g_widget_total = NULL;
static lv_obj_t *g_widgets_container = NULL;

// Controle do botão físico
static uint32_t g_last_button_time = 0;

// Declarações de funções privadas
static void save_button_cb(lv_event_t *e);
static void cancel_button_cb(lv_event_t *e);
static void history_button_cb(lv_event_t *e);
static void history_close_cb(void);
static void update_ui_state(void);
static float read_lidar_distance(void);
static void update_result_display(void);
static void feedback_timer_cb(lv_timer_t *timer);
// static void calculation_timer_cb(lv_timer_t *timer); // DESABILITADO - usando botão físico
static void lidar_update_timer_cb(lv_timer_t *timer);
static void create_horizontal_widget(void);
static void create_top_widget(void);
static void create_base_widget(void);
static void calculate_height1(void);
static void calculate_height2(void);
static void calculate_total_height(void);
static void show_action_buttons(void);
static void clear_all_widgets(void);
static bool check_button_press(void);
static void show_error_feedback(const char *message);
static void save_and_restart(void);

// Textos das instruções para cada estado
static const char* get_instruction_text(plant_measurement_state_t state) {
    switch (state) {
        case PLANT_MEASUREMENT_STATE_IDLE:
            return g_mode == MEASUREMENT_MODE_SINGLE ?
                   "Pressione para iniciar MEDICAO UNICA" :
                   "Pressione o botao fisico para iniciar medicao";
        case PLANT_MEASUREMENT_STATE_HORIZONTAL:
            return g_mode == MEASUREMENT_MODE_SINGLE ?
                   "Mire na planta e pressione para leitura UNICA" :
                   "PASSO 1: Mire no CENTRO da planta e pressione o botao";
        case PLANT_MEASUREMENT_STATE_TOP:
            return "PASSO 2: Mire no TOPO da planta e pressione o botao";
        case PLANT_MEASUREMENT_STATE_BASE:
            return "PASSO 3: Mire na BASE da planta e pressione o botao";
        case PLANT_MEASUREMENT_STATE_CALCULATING:
            return "Calculando altura total...";
        case PLANT_MEASUREMENT_STATE_COMPLETE:
            return "Concluido! Salve ou pressione botao para nova medicao";
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

    g_measurement_screen = lv_obj_create(parent);
    lv_obj_set_size(g_measurement_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_measurement_screen, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_measurement_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_measurement_screen, 0, LV_PART_MAIN);
    
    // Desabilitar scroll na tela de medição
    lv_obj_clear_flag(g_measurement_screen, LV_OBJ_FLAG_SCROLLABLE);

    // === CABEÇALHO ===
    lv_obj_t *header = lv_obj_create(g_measurement_screen);
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

    // Botão de histórico no cabeçalho
    g_history_btn = lv_btn_create(header);
    lv_obj_set_size(g_history_btn, 100, 40);
    lv_obj_align(g_history_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_radius(g_history_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_history_btn, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_history_btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_add_event_cb(g_history_btn, history_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *history_label = lv_label_create(g_history_btn);
    lv_label_set_text(history_label, LV_SYMBOL_LIST " Hist");
    lv_obj_set_style_text_color(history_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(history_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(history_label);

    // === PAINEL DE STATUS ===
    lv_obj_t *status_panel = lv_obj_create(g_measurement_screen);
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
    lv_obj_t *instruction_panel = lv_obj_create(g_measurement_screen);
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
    g_result_panel = lv_obj_create(g_measurement_screen);
    lv_obj_set_size(g_result_panel, 456, 120);
    lv_obj_set_pos(g_result_panel, 12, 230);
    lv_obj_set_style_bg_color(g_result_panel, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_result_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_result_panel, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius(g_result_panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_result_panel, 15, LV_PART_MAIN);
    lv_obj_add_flag(g_result_panel, LV_OBJ_FLAG_HIDDEN); // Oculto inicialmente

    // === CONTAINER PARA WIDGETS DINÂMICOS ===
    g_widgets_container = lv_obj_create(g_measurement_screen);
    lv_obj_set_size(g_widgets_container, 456, 120);
    lv_obj_set_pos(g_widgets_container, 12, 230);
    lv_obj_set_style_bg_opa(g_widgets_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_widgets_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widgets_container, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_widgets_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_widgets_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(g_widgets_container, LV_OBJ_FLAG_SCROLLABLE);

    // === BOTÕES DE AÇÃO (OCULTOS INICIALMENTE) ===

    g_save_btn = lv_btn_create(g_measurement_screen);
    lv_obj_set_size(g_save_btn, 120, 45);
    // Posicionar ancorado ao rodapé para garantir visibilidade em qualquer rotação
    lv_obj_align(g_save_btn, LV_ALIGN_BOTTOM_LEFT, 30, -12);
    lv_obj_set_style_bg_color(g_save_btn, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_add_event_cb(g_save_btn, save_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN); // Oculto inicialmente

    lv_obj_t *save_label = lv_label_create(g_save_btn);
    lv_label_set_text(save_label, "SALVAR");
    lv_obj_set_style_text_color(save_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(save_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(save_label);

    g_cancel_btn = lv_btn_create(g_measurement_screen);
    lv_obj_set_size(g_cancel_btn, 120, 45);
    // Posicionar ancorado ao rodapé para garantir visibilidade em qualquer rotação
    lv_obj_align(g_cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -12);
    lv_obj_set_style_bg_color(g_cancel_btn, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
    lv_obj_add_event_cb(g_cancel_btn, cancel_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_cancel_btn, LV_OBJ_FLAG_HIDDEN); // Oculto inicialmente

    lv_obj_t *cancel_label = lv_label_create(g_cancel_btn);
    lv_label_set_text(cancel_label, "CANCELAR");
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(cancel_label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(cancel_label);

    // Resetar estado inicial
    g_state = PLANT_MEASUREMENT_STATE_IDLE;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    update_ui_state();

    // Configurar botão físico
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << MEASUREMENT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_config);

    // Iniciar timer para atualizar leitura do LiDAR e monitorar botão (a cada 200ms para melhor responsividade)
    g_lidar_update_timer = lv_timer_create(lidar_update_timer_cb, 200, NULL);

    ESP_LOGI(TAG, "Measurement screen created successfully with physical button on GPIO %d", MEASUREMENT_BUTTON_PIN);
    return g_measurement_screen;
}

// === IMPLEMENTAÇÃO DOS CALLBACKS ===

static void process_button_press(void)
{
    ESP_LOGI(TAG, "Physical button pressed, current state: %d", g_state);
    
    switch (g_state) {
        case PLANT_MEASUREMENT_STATE_IDLE:
            g_state = PLANT_MEASUREMENT_STATE_HORIZONTAL;
            break;
            
        case PLANT_MEASUREMENT_STATE_HORIZONTAL:
            {
                float distance = read_lidar_distance();
                if (distance > 0.0f) {
                    if (g_mode == MEASUREMENT_MODE_SINGLE) {
                        // Medição única: usar a leitura como altura total
                        g_measurement.distance_horizontal = distance;
                        g_measurement.distance_to_top = distance;
                        g_measurement.distance_to_base = distance;
                        g_measurement.height_top = distance;
                        g_measurement.height_base = 0.0f;
                        g_measurement.total_height = distance;
                        g_measurement.measurement_valid = true;

                        ESP_LOGI(TAG, "Single measurement captured: %.1f cm", distance);

                        g_state = PLANT_MEASUREMENT_STATE_COMPLETE;
                        show_action_buttons();
                    } else {
                        g_measurement.distance_horizontal = distance;
                        ESP_LOGI(TAG, "Horizontal distance captured: %.1f cm", g_measurement.distance_horizontal);
                        create_horizontal_widget();
                        g_state = PLANT_MEASUREMENT_STATE_TOP;
                    }
                } else {
                    ESP_LOGW(TAG, "Invalid LiDAR reading, try again");
                    show_error_feedback("LiDAR sem sinal! Tente novamente");
                }
            }
            break;
            
        case PLANT_MEASUREMENT_STATE_TOP:
            {
                float distance = read_lidar_distance();
                if (distance > 0.0f) {
                    g_measurement.distance_to_top = distance;
                    ESP_LOGI(TAG, "Distance to top captured: %.1f cm", g_measurement.distance_to_top);
                    create_top_widget();
                    calculate_height1();
                    g_state = PLANT_MEASUREMENT_STATE_BASE;
                } else {
                    ESP_LOGW(TAG, "Invalid LiDAR reading, try again");
                    show_error_feedback("LiDAR sem sinal! Tente novamente");
                }
            }
            break;
            
        case PLANT_MEASUREMENT_STATE_BASE:
            {
                float distance = read_lidar_distance();
                if (distance > 0.0f) {
                    g_measurement.distance_to_base = distance;
                    ESP_LOGI(TAG, "Distance to base captured: %.1f cm", g_measurement.distance_to_base);
                    create_base_widget();
                    calculate_height2();
                    calculate_total_height();
                    g_state = PLANT_MEASUREMENT_STATE_COMPLETE;
                } else {
                    ESP_LOGW(TAG, "Invalid LiDAR reading, try again");
                    show_error_feedback("LiDAR sem sinal! Tente novamente");
                }
            }
            show_action_buttons();
            break;
            
        case PLANT_MEASUREMENT_STATE_COMPLETE:
            // Requisito: botão físico no estado COMPLETO deve SALVAR e iniciar nova medição
            ESP_LOGI(TAG, "Complete -> salvar e iniciar nova medicao (botao fisico)");
            save_and_restart();
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
        // Requisito: no botão virtual "Salvar" também deve salvar e iniciar nova medição
        save_and_restart();
    }
}

static void cancel_button_cb(lv_event_t *e)
{
    (void)e;
    
    ESP_LOGI(TAG, "Measurement cancelled");
    
    // Limpar todos os widgets dinâmicos
    clear_all_widgets();
    
    // Requisito: "Cancelar" inicia uma nova medição sem salvar
    g_state = PLANT_MEASUREMENT_STATE_HORIZONTAL;
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    update_ui_state();
}

static void history_button_cb(lv_event_t *e)
{
    (void)e;
    
    ESP_LOGI(TAG, "Opening measurement history");
    
    if (g_history_subscreen) {
        lv_obj_del(g_history_subscreen);
        g_history_subscreen = NULL;
    }
    
    if (g_measurement_screen) {
        lv_obj_add_flag(g_measurement_screen, LV_OBJ_FLAG_HIDDEN);
    }
    
    g_history_subscreen = measurement_history_create(lv_obj_get_parent(g_measurement_screen), history_close_cb);
}

static void history_close_cb(void)
{
    ESP_LOGI(TAG, "Closing measurement history");
    
    if (g_history_subscreen) {
        lv_obj_del_async(g_history_subscreen);
        g_history_subscreen = NULL;
    }
    
    if (g_measurement_screen) {
        lv_obj_clear_flag(g_measurement_screen, LV_OBJ_FLAG_HIDDEN);
    }
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
                // Durante medição horizontal, mostrar LiDAR em tempo real
                if (lidar_available && current_lidar.valid) {
                    snprintf(dist_text, sizeof(dist_text), "Medindo Horizontal: %d cm (Pressione para capturar)", 
                            current_lidar.distance);
                } else {
                    snprintf(dist_text, sizeof(dist_text), "Medindo Horizontal: -- cm (Sem sinal)");
                }
                break;
            case PLANT_MEASUREMENT_STATE_TOP:
                // Durante medição do topo, mostrar LiDAR em tempo real
                if (lidar_available && current_lidar.valid) {
                    snprintf(dist_text, sizeof(dist_text), "Medindo ao Topo: %d cm (Pressione para capturar)", 
                            current_lidar.distance);
                } else {
                    snprintf(dist_text, sizeof(dist_text), "Medindo ao Topo: -- cm (Sem sinal)");
                }
                break;
            case PLANT_MEASUREMENT_STATE_BASE:
                // Durante medição da base, mostrar LiDAR em tempo real
                if (lidar_available && current_lidar.valid) {
                    snprintf(dist_text, sizeof(dist_text), "Medindo a Base: %d cm (Pressione para capturar)", 
                            current_lidar.distance);
                } else {
                    snprintf(dist_text, sizeof(dist_text), "Medindo a Base: -- cm (Sem sinal)");
                }
                break;
            case PLANT_MEASUREMENT_STATE_COMPLETE:
                snprintf(dist_text, sizeof(dist_text), "Medicao Completa: %.1f cm total", 
                        g_measurement.total_height);
                break;
            default:
                if (lidar_available && current_lidar.valid) {
                    snprintf(dist_text, sizeof(dist_text), "Distancia: %d cm", 
                            current_lidar.distance);
                } else {
                    snprintf(dist_text, sizeof(dist_text), "Distancia: -- cm");
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
            lv_obj_move_foreground(g_save_btn);
        } else {
            lv_obj_add_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (g_cancel_btn) {
        if (g_state == PLANT_MEASUREMENT_STATE_COMPLETE && g_measurement.measurement_valid) {
            lv_obj_clear_flag(g_cancel_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(g_cancel_btn);
        } else {
            lv_obj_add_flag(g_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // O botão físico substituiu o botão touch - não precisamos mais atualizar UI do botão
    // A lógica agora é controlada pelo botão físico no GPIO 5
}



static float read_lidar_distance(void)
{
    lidar_data_t lidar_data;
    
    // Usar a mesma função que funciona no display dinâmico
    if (lidar_get_last_reading(&lidar_data) && lidar_data.valid) {
        ESP_LOGI(TAG, "Captured LiDAR reading: %d cm, strength: %d", 
                 lidar_data.distance, lidar_data.strength);
        
        // Verificar se a leitura está dentro dos limites esperados
        if (lidar_data.distance >= 10 && lidar_data.distance <= 1200) {
            return (float)lidar_data.distance;
        } else {
            ESP_LOGW(TAG, "LiDAR distance out of range: %d cm", lidar_data.distance);
            return 0.0f; // Retorna 0 para indicar erro
        }
    } else {
        ESP_LOGW(TAG, "No valid LiDAR data available");
        return 0.0f; // Retorna 0 para indicar erro
    }
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

static void lidar_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    // Verificar se o botão foi pressionado
    if (check_button_press()) {
        process_button_press();
    }
    
    // Atualizar apenas se estivermos em estado idle ou aguardando medição
    if (g_state == PLANT_MEASUREMENT_STATE_IDLE || 
        g_state == PLANT_MEASUREMENT_STATE_HORIZONTAL ||
        g_state == PLANT_MEASUREMENT_STATE_TOP ||
        g_state == PLANT_MEASUREMENT_STATE_BASE) {
        
        // Atualizar o display de distância dinâmico durante medições
        if (g_distance_label) {
            char dist_text[128];
            lidar_data_t current_lidar;
            
            if (lidar_get_last_reading(&current_lidar) && current_lidar.valid) {
                switch (g_state) {
                    case PLANT_MEASUREMENT_STATE_IDLE:
                        snprintf(dist_text, sizeof(dist_text), "Distancia: %d cm", 
                                current_lidar.distance);
                        break;
                        
                    case PLANT_MEASUREMENT_STATE_HORIZONTAL:
                        snprintf(dist_text, sizeof(dist_text), "Medindo HORIZONTAL: %d cm (Clique para capturar)", 
                                current_lidar.distance);
                        break;
                        
                    case PLANT_MEASUREMENT_STATE_TOP:
                        snprintf(dist_text, sizeof(dist_text), "Medindo TOPO: %d cm (Clique para capturar)", 
                                current_lidar.distance);
                        break;
                        
                    case PLANT_MEASUREMENT_STATE_BASE:
                        snprintf(dist_text, sizeof(dist_text), "Medindo BASE: %d cm (Clique para capturar)", 
                                current_lidar.distance);
                        break;
                        
                    default:
                        snprintf(dist_text, sizeof(dist_text), "LiDAR: %d cm", current_lidar.distance);
                        break;
                }
                lv_label_set_text(g_distance_label, dist_text);
            } else {
                // Sem sinal do LiDAR
                lv_label_set_text(g_distance_label, "LiDAR: Sem sinal");
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

void measurement_set_mode(measurement_mode_t mode)
{
    g_mode = mode;
    ESP_LOGI(TAG, "Measurement mode set to: %s", mode == MEASUREMENT_MODE_SINGLE ? "UNICA" : "COMPLETA");
}

measurement_mode_t measurement_get_mode(void)
{
    return g_mode;
}

// === FUNÇÕES DOS WIDGETS DINÂMICOS ===

static void create_horizontal_widget(void)
{
    if (!g_widgets_container) return;
    
    g_widget_horizontal = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_horizontal, 440, 30);
    lv_obj_set_style_bg_color(g_widget_horizontal, lv_color_hex(UI_COLOR_PRIMARY_VARIANT), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_horizontal, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_horizontal, 8, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_horizontal);
    char text[100];
    snprintf(text, sizeof(text), "Distancia Horizontal: %.1f cm", g_measurement.distance_horizontal);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Created horizontal distance widget");
}

static void create_top_widget(void)
{
    if (!g_widgets_container) return;
    
    g_widget_top = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_top, 440, 30);
    lv_obj_set_style_bg_color(g_widget_top, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_top, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_top, 8, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_top);
    char text[100];
    snprintf(text, sizeof(text), "Distancia ao Topo: %.1f cm", g_measurement.distance_to_top);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Created top distance widget");
}

static void create_base_widget(void)
{
    if (!g_widgets_container) return;
    
    g_widget_base = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_base, 440, 30);
    lv_obj_set_style_bg_color(g_widget_base, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_base, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_base, 8, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_base);
    char text[100];
    snprintf(text, sizeof(text), "Distancia a Base: %.1f cm", g_measurement.distance_to_base);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Created base distance widget");
}

static void calculate_height1(void)
{
    // Calcular altura 1 (do horizontal ao topo)
    float dist_top_sq = g_measurement.distance_to_top * g_measurement.distance_to_top;
    float dist_hor_sq = g_measurement.distance_horizontal * g_measurement.distance_horizontal;
    
    if (dist_top_sq > dist_hor_sq) {
        g_measurement.height_top = sqrt(dist_top_sq - dist_hor_sq);
    } else {
        g_measurement.height_top = 0.0f;
    }
    
    // Criar widget para altura 1
    if (!g_widgets_container) return;
    
    g_widget_height1 = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_height1, 440, 30);
    lv_obj_set_style_bg_color(g_widget_height1, lv_color_hex(UI_COLOR_WARNING), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_height1, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_height1, 8, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_height1);
    char text[100];
    snprintf(text, sizeof(text), "Altura 1 (topo): %.1f cm", g_measurement.height_top);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Calculated and created height1 widget: %.1f cm", g_measurement.height_top);
}

static void calculate_height2(void)
{
    // Calcular altura 2 (do horizontal à base)
    float dist_base_sq = g_measurement.distance_to_base * g_measurement.distance_to_base;
    float dist_hor_sq = g_measurement.distance_horizontal * g_measurement.distance_horizontal;
    
    if (dist_base_sq > dist_hor_sq) {
        g_measurement.height_base = sqrt(dist_base_sq - dist_hor_sq);
    } else {
        g_measurement.height_base = 0.0f;
    }
    
    // Criar widget para altura 2
    if (!g_widgets_container) return;
    
    g_widget_height2 = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_height2, 440, 30);
    lv_obj_set_style_bg_color(g_widget_height2, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_height2, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_height2, 8, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_height2);
    char text[100];
    snprintf(text, sizeof(text), "Altura 2 (base): %.1f cm", g_measurement.height_base);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Calculated and created height2 widget: %.1f cm", g_measurement.height_base);
}

static void calculate_total_height(void)
{
    // Calcular altura total
    g_measurement.total_height = g_measurement.height_top + g_measurement.height_base;
    g_measurement.measurement_valid = true;
    
    // Criar widget para altura total (destacado)
    if (!g_widgets_container) return;
    
    g_widget_total = lv_obj_create(g_widgets_container);
    lv_obj_set_size(g_widget_total, 440, 40);
    lv_obj_set_style_bg_color(g_widget_total, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
    lv_obj_set_style_radius(g_widget_total, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_widget_total, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_widget_total, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_widget_total, LV_OPA_30, LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(g_widget_total);
    char text[100];
    snprintf(text, sizeof(text), "ALTURA TOTAL: %.1f cm", g_measurement.total_height);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_center(label);
    
    ESP_LOGI(TAG, "Calculated total height: %.1f cm", g_measurement.total_height);
}

static void show_action_buttons(void)
{
    // Mostrar botões de salvar e cancelar
    if (g_save_btn) {
        lv_obj_clear_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_save_btn);
    }
    if (g_cancel_btn) {
        lv_obj_clear_flag(g_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_cancel_btn);
    }
    ESP_LOGI(TAG, "Showing action buttons");
}

static void clear_all_widgets(void)
{
    // Limpar todos os widgets dinâmicos
    if (g_widgets_container) {
        lv_obj_clean(g_widgets_container);
    }
    
    g_widget_horizontal = NULL;
    g_widget_top = NULL;
    g_widget_base = NULL;
    g_widget_height1 = NULL;
    g_widget_height2 = NULL;
    g_widget_total = NULL;
    
    // Ocultar botões de ação
    if (g_save_btn) {
        lv_obj_add_flag(g_save_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (g_cancel_btn) {
        lv_obj_add_flag(g_cancel_btn, LV_OBJ_FLAG_HIDDEN);
    }
    
    ESP_LOGI(TAG, "Cleared all measurement widgets");
}

static bool check_button_press(void)
{
    static bool last_button_state = true; // Pull-up = nível alto quando solto
    int button_level = gpio_get_level(MEASUREMENT_BUTTON_PIN);
    uint32_t current_time = esp_timer_get_time() / 1000; // ms
    
    // Detecta borda de descida (pressionado) com debounce melhorado
    if (button_level == 0 && last_button_state == true && 
        (current_time - g_last_button_time > 250)) { // Debounce de 250ms
        
        g_last_button_time = current_time;
        last_button_state = false;
        
        ESP_LOGI(TAG, "Botão físico pressionado! Estado: %d", g_state);
        return true;
    }
    
    // Atualiza estado do botão
    if (button_level == 1) {
        last_button_state = true;
    }
    
    return false;
}

static void show_error_feedback(const char *message)
{
    if (!g_widgets_container || !message) return;
    
    // Criar feedback temporário de erro
    lv_obj_t *error_label = lv_label_create(g_widgets_container);
    lv_label_set_text(error_label, message);
    lv_obj_set_style_text_color(error_label, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
    lv_obj_set_style_text_font(error_label, UI_FONT_MEDIUM, LV_PART_MAIN);
    lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
    
    // Remover após 2 segundos usando timer simples
    static lv_obj_t *temp_error_label = NULL;
    if (temp_error_label) {
        lv_obj_del(temp_error_label);
    }
    temp_error_label = error_label;
    
    ESP_LOGI(TAG, "Error feedback: %s", message);
}

// === SALVAR E REINICIAR ===
static void save_and_restart(void)
{
    // Monta timestamp legível
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    strftime(g_measurement.timestamp, sizeof(g_measurement.timestamp), "%d/%m/%Y %H:%M", timeinfo);

    // Monta linha CSV (plant_name vazio inicialmente)
    char line[512];
    snprintf(line, sizeof(line), "%s,,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
             g_measurement.timestamp,
             g_measurement.distance_horizontal,
             g_measurement.distance_to_top,
             g_measurement.distance_to_base,
             g_measurement.height_top,
             g_measurement.height_base,
             g_measurement.total_height);

    const char *csv_path = SD_MOUNT_POINT "/medicoes.csv";
    const char *csv_header = "timestamp,plant_name,horizontal_cm,top_cm,base_cm,height_top_cm,height_base_cm,total_cm";

    // Onde mostrar feedback
    lv_obj_t *parent = g_save_btn ? lv_obj_get_parent(g_save_btn) : NULL;
    if (!parent && g_widgets_container) parent = g_widgets_container;
    if (!parent && g_distance_label) parent = lv_obj_get_parent(g_distance_label);
    lv_obj_t *feedback = parent ? lv_label_create(parent) : NULL;

    if (sd_storage_is_mounted()) {
        esp_err_t err = sd_storage_append_csv(csv_path, csv_header, line);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Measurement saved to %s: %s", csv_path, line);
            
            // Feedback inicial de salvamento
            if (feedback) {
                lv_label_set_text(feedback, "Salvo no SD!");
                lv_obj_set_style_text_color(feedback, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
                lv_obj_set_style_text_font(feedback, UI_FONT_MEDIUM, LV_PART_MAIN);
                lv_obj_align(feedback, LV_ALIGN_BOTTOM_MID, 0, -10);
            }
            
            // Enviar para Supabase
            ESP_LOGI(TAG, "Checking Supabase readiness...");
            if (supabase_is_ready()) {
                ESP_LOGI(TAG, "Supabase is ready, preparing data...");
                
                // Atualizar feedback para mostrar envio
                if (feedback) {
                    lv_label_set_text(feedback, "Salvo! Enviando ao servidor...");
                    lv_obj_set_style_text_color(feedback, lv_color_hex(0x1E90FF), LV_PART_MAIN); // Azul
                }
                
                // Formatar timestamp para ISO8601 (Supabase espera formato PostgreSQL)
                char iso_timestamp[32];
                strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
                ESP_LOGI(TAG, "Timestamp formatted: %s", iso_timestamp);
                
                supabase_measurement_t supabase_data = {
                    .horizontal_cm = g_measurement.distance_horizontal,
                    .top_cm = g_measurement.distance_to_top,
                    .base_cm = g_measurement.distance_to_base,
                    .height_top_cm = g_measurement.height_top,
                    .height_base_cm = g_measurement.height_base,
                    .total_cm = g_measurement.total_height
                };
                strncpy(supabase_data.timestamp, iso_timestamp, sizeof(supabase_data.timestamp) - 1);
                
                ESP_LOGI(TAG, "Calling supabase_send_measurement()...");
                esp_err_t supabase_err = supabase_send_measurement(&supabase_data);
                if (supabase_err == ESP_OK) {
                    ESP_LOGI(TAG, "Measurement queued for Supabase successfully");
                    if (feedback) {
                        lv_label_set_text(feedback, "Salvo! Enviando ao servidor...");
                        lv_timer_create(feedback_timer_cb, 3000, feedback);
                    }
                } else {
                    ESP_LOGW(TAG, "Failed to queue measurement for Supabase: %s", esp_err_to_name(supabase_err));
                    if (feedback) {
                        lv_label_set_text(feedback, "Salvo! Erro ao enviar servidor");
                        lv_obj_set_style_text_color(feedback, lv_color_hex(0xFFA500), LV_PART_MAIN); // Laranja
                        lv_timer_create(feedback_timer_cb, 3000, feedback);
                    }
                }
            } else {
                ESP_LOGW(TAG, "Supabase not initialized, skipping cloud sync");
                if (feedback) {
                    lv_label_set_text(feedback, "Salvo! (Servidor offline)");
                    lv_obj_set_style_text_color(feedback, lv_color_hex(0xFFA500), LV_PART_MAIN); // Laranja
                    lv_timer_create(feedback_timer_cb, 3000, feedback);
                }
            }
        } else {
            ESP_LOGE(TAG, "Falha ao salvar no SD (%s)", esp_err_to_name(err));
            if (feedback) {
                lv_label_set_text(feedback, "Falha ao salvar no SD");
                lv_obj_set_style_text_color(feedback, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
                lv_obj_align(feedback, LV_ALIGN_BOTTOM_MID, 0, -10);
                lv_timer_create(feedback_timer_cb, 2000, feedback);
            }
        }
    } else {
        ESP_LOGW(TAG, "Cartão SD não montado, não foi possível salvar");
        if (feedback) {
            lv_label_set_text(feedback, "Cartão não detectado");
            lv_obj_set_style_text_color(feedback, lv_color_hex(UI_COLOR_ERROR), LV_PART_MAIN);
            lv_obj_align(feedback, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_timer_create(feedback_timer_cb, 2000, feedback);
        }
    }

    // Reiniciar nova medição
    clear_all_widgets();
    memset(&g_measurement, 0, sizeof(plant_measurement_t));
    g_state = PLANT_MEASUREMENT_STATE_HORIZONTAL;
    update_ui_state();
}
