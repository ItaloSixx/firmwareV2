/**
 * @file ui_os.c
 * @brief Sistema Operacional Principal - Implementação ESP32-S3
 * @author ItaloSixx
 * @date 2025
 */

#include "ui_os.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "UI_OS";

// =============================================================================
// VARIÁVEIS GLOBAIS
// =============================================================================

// Objetos principais da interface
static lv_obj_t *main_container = NULL;
static lv_obj_t *status_bar = NULL;
static lv_obj_t *content_area = NULL;
static lv_obj_t *nav_bar = NULL;

// Telas do sistema
static lv_obj_t *screens[UI_SCREEN_COUNT] = {NULL};
static ui_screen_t current_screen = UI_SCREEN_HOME;

// Estado do sistema
static ui_system_state_t system_state = {0};
static ui_system_stats_t system_stats = {0};

// Labels da status bar
static lv_obj_t *time_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *wifi_icon = NULL;
static lv_obj_t *bluetooth_icon = NULL;
static lv_obj_t *notification_icon = NULL;

// Botões da navigation bar
static lv_obj_t *nav_buttons[UI_SCREEN_COUNT] = {NULL};

// Área de notificação
static lv_obj_t *notification_area = NULL;
static lv_timer_t *notification_timer = NULL;

// =============================================================================
// FUNÇÕES DE ESTILO
// =============================================================================

/*
 * @brief Aplica estilo moderno para botao primario (reservado para uso futuro)
 */
/*
static void apply_button_primary_style(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_PRIMARY_VARIANT), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, UI_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 4, LV_PART_MAIN);
}
*/

/**
 * @brief Aplica estilo moderno para card/container
 */
static void apply_card_style(lv_obj_t *card)
{
    lv_obj_set_style_bg_color(card, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(card, UI_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, UI_MARGIN_MEDIUM, LV_PART_MAIN);
}

// =============================================================================
// CALLBACKS DE EVENTOS
// =============================================================================

/**
 * @brief Callback para mudanca de tela
 */
static void nav_button_cb(lv_event_t *e)
{
    ui_screen_t screen = (ui_screen_t)(uintptr_t)lv_event_get_user_data(e);
    
    if (screen != current_screen) {
        ui_set_screen(screen);
        ESP_LOGI(TAG, "Mudando para tela: %d", screen);
    }
}

/**
 * @brief Callback para timer de notificação
 */
static void notification_timer_cb(lv_timer_t *timer)
{
    if (notification_area) {
        lv_obj_add_flag(notification_area, LV_OBJ_FLAG_HIDDEN);
        lv_timer_del(timer);
        notification_timer = NULL;
    }
}

// =============================================================================
// CRIAÇÃO DE COMPONENTES
// =============================================================================

/**
 * @brief Cria a barra de status superior
 */
static void create_status_bar(void)
{
    status_bar = lv_obj_create(main_container);
    lv_obj_set_size(status_bar, UI_SCREEN_WIDTH, UI_STATUS_BAR_HEIGHT);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(UI_COLOR_PRIMARY_VARIANT), LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar, 4, LV_PART_MAIN);
    
    // Horário
    time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 8, 0);
    
    // Título central
    lv_obj_t *title_label = lv_label_create(status_bar);
    lv_label_set_text(title_label, "ESP32-S3 OS");
    lv_obj_set_style_text_color(title_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(title_label);
    
    // Bateria
    battery_label = lv_label_create(status_bar);
    lv_label_set_text(battery_label, "100%");
    lv_obj_set_style_text_color(battery_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -8, 0);
    
    // Ícones de conectividade
    wifi_icon = lv_label_create(status_bar);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -60, 0);
    
    bluetooth_icon = lv_label_create(status_bar);
    lv_label_set_text(bluetooth_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(bluetooth_icon, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
    lv_obj_align(bluetooth_icon, LV_ALIGN_RIGHT_MID, -80, 0);
    
    // Notificações
    notification_icon = lv_label_create(status_bar);
    lv_label_set_text(notification_icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(notification_icon, lv_color_hex(UI_COLOR_WARNING), LV_PART_MAIN);
    lv_obj_align(notification_icon, LV_ALIGN_RIGHT_MID, -100, 0);
    lv_obj_add_flag(notification_icon, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Cria a barra de navegação inferior
 */
static void create_navigation_bar(void)
{
    nav_bar = lv_obj_create(main_container);
    lv_obj_set_size(nav_bar, UI_SCREEN_WIDTH, UI_NAV_BAR_HEIGHT);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_color_hex(UI_COLOR_SURFACE), LV_PART_MAIN);
    lv_obj_set_style_radius(nav_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(nav_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(nav_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_side(nav_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nav_bar, 4, LV_PART_MAIN);
    
    // Configuração dos botões
    const char *nav_icons[] = {LV_SYMBOL_HOME, "🔧", LV_SYMBOL_SETTINGS, LV_SYMBOL_LIST};
    const char *nav_labels[] = {"Home", "Sensores", "Config", "Sobre"};
    
    int btn_width = (UI_SCREEN_WIDTH - (5 * 4)) / 4; // Largura disponível dividida pelos botões
    
    for (int i = 0; i < UI_SCREEN_COUNT; i++) {
        // Container do botão
        lv_obj_t *btn_container = lv_obj_create(nav_bar);
        lv_obj_set_size(btn_container, btn_width, UI_NAV_BAR_HEIGHT - 8);
        lv_obj_align(btn_container, LV_ALIGN_LEFT_MID, 4 + i * (btn_width + 4), 0);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn_container, 2, LV_PART_MAIN);
        lv_obj_add_flag(btn_container, LV_OBJ_FLAG_CLICKABLE);
        
        // Ícone
        lv_obj_t *icon = lv_label_create(btn_container);
        lv_label_set_text(icon, nav_icons[i]);
        lv_obj_set_style_text_color(icon, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 2);
        
        // Label
        lv_obj_t *label = lv_label_create(btn_container);
        lv_label_set_text(label, nav_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -2);
        
        // Evento de clique
        lv_obj_add_event_cb(btn_container, nav_button_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        
        nav_buttons[i] = btn_container;
    }
}

/**
 * @brief Cria a área de conteúdo principal
 */
static void create_content_area(void)
{
    content_area = lv_obj_create(main_container);
    lv_obj_set_size(content_area, UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_align(content_area, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(content_area, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_radius(content_area, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content_area, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content_area, 0, LV_PART_MAIN);
}

// =============================================================================
// CRIAÇÃO DAS TELAS
// =============================================================================

/**
 * @brief Cria a tela Home moderna
 */
static void create_home_screen(void)
{
    screens[UI_SCREEN_HOME] = lv_obj_create(content_area);
    lv_obj_set_size(screens[UI_SCREEN_HOME], UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(screens[UI_SCREEN_HOME], LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(screens[UI_SCREEN_HOME], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screens[UI_SCREEN_HOME], UI_MARGIN_LARGE, LV_PART_MAIN);
    
    // Titulo principal centrado
    lv_obj_t *main_title = lv_label_create(screens[UI_SCREEN_HOME]);
    lv_label_set_text(main_title, "Que deseja fazer hoje?");
    lv_obj_set_style_text_color(main_title, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(main_title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(main_title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Container para os botoes principais (grid 2x2)
    lv_obj_t *button_container = lv_obj_create(screens[UI_SCREEN_HOME]);
    lv_obj_set_size(button_container, 440, 140);
    lv_obj_align(button_container, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_opa(button_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button_container, 0, LV_PART_MAIN);
    
    // Definir cores para cada botao (como na imagem)
    uint32_t button_colors[] = {
        0x8E44AD, // Purple - Sensores
        0x3498DB, // Blue - Sistema  
        0x27AE60, // Green - Config
        0xE74C3C  // Red - Sobre
    };
    
    const char* button_texts[] = {
        "SENSORES",
        "SISTEMA", 
        "CONFIG",
        "SOBRE"
    };
    
    // Criar botoes em grid 2x2
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(button_container);
        lv_obj_set_size(btn, 100, 60);
        
        // Posicionar em grid
        int x = (i % 2) * 110 + 10;
        int y = (i / 2) * 70 + 10;
        lv_obj_set_pos(btn, x, y);
        
        // Aplicar cor especifica
        lv_obj_set_style_bg_color(btn, lv_color_hex(button_colors[i]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, UI_RADIUS_LARGE, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
        lv_obj_set_style_shadow_ofs_y(btn, 4, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(button_colors[i]), LV_PART_MAIN);
        
        // Texto do botao
        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, button_texts[i]);
        lv_obj_set_style_text_color(btn_label, lv_color_hex(UI_COLOR_ON_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_center(btn_label);
        
        // Callback para navegacao
        lv_obj_add_event_cb(btn, nav_button_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
}

/**
 * @brief Cria a tela de Sensores
 */
static void create_sensors_screen(void)
{
    screens[UI_SCREEN_SENSORS] = lv_obj_create(content_area);
    lv_obj_set_size(screens[UI_SCREEN_SENSORS], UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(screens[UI_SCREEN_SENSORS], LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(screens[UI_SCREEN_SENSORS], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screens[UI_SCREEN_SENSORS], UI_MARGIN_MEDIUM, LV_PART_MAIN);
    lv_obj_add_flag(screens[UI_SCREEN_SENSORS], LV_OBJ_FLAG_HIDDEN);
    
    // Titulo
    lv_obj_t *title = lv_label_create(screens[UI_SCREEN_SENSORS]);
    lv_label_set_text(title, "Monitoramento de Sensores");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Card BNO055 (lado esquerdo)
    lv_obj_t *bno_card = lv_obj_create(screens[UI_SCREEN_SENSORS]);
    lv_obj_set_size(bno_card, 220, 180);
    lv_obj_align(bno_card, LV_ALIGN_TOP_LEFT, 10, 40);
    apply_card_style(bno_card);
    
    lv_obj_t *bno_title = lv_label_create(bno_card);
    lv_label_set_text(bno_title, LV_SYMBOL_CHARGE " BNO055 - Orientacao");
    lv_obj_set_style_text_color(bno_title, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
    lv_obj_set_style_text_font(bno_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(bno_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *bno_data = lv_label_create(bno_card);
    lv_label_set_text(bno_data, "Pitch: 0.0 graus\\nRoll: 0.0 graus\\nYaw: 0.0 graus\\nTemp: -- C\\nStatus: Conectando...");
    lv_obj_set_style_text_color(bno_data, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(bno_data, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(bno_data, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // Card de outros sensores (lado direito)
    lv_obj_t *other_card = lv_obj_create(screens[UI_SCREEN_SENSORS]);
    lv_obj_set_size(other_card, 220, 180);
    lv_obj_align(other_card, LV_ALIGN_TOP_RIGHT, -10, 40);
    apply_card_style(other_card);
    
    lv_obj_t *other_title = lv_label_create(other_card);
    lv_label_set_text(other_title, LV_SYMBOL_SETTINGS " Outros Sensores");
    lv_obj_set_style_text_color(other_title, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
    lv_obj_set_style_text_font(other_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(other_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *other_data = lv_label_create(other_card);
    lv_label_set_text(other_data, "Temperatura: -- C\\nUmidade: -- %\\nPressao: -- hPa\\nLuz: -- lux\\nI2C: Ativo");
    lv_obj_set_style_text_color(other_data, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(other_data, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(other_data, LV_ALIGN_TOP_LEFT, 0, 25);
}

/**
 * @brief Cria a tela de Configurações
 */
static void create_settings_screen(void)
{
    screens[UI_SCREEN_SETTINGS] = lv_obj_create(content_area);
    lv_obj_set_size(screens[UI_SCREEN_SETTINGS], UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(screens[UI_SCREEN_SETTINGS], LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(screens[UI_SCREEN_SETTINGS], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screens[UI_SCREEN_SETTINGS], UI_MARGIN_MEDIUM, LV_PART_MAIN);
    lv_obj_add_flag(screens[UI_SCREEN_SETTINGS], LV_OBJ_FLAG_HIDDEN);
    
    // Título
    lv_obj_t *title = lv_label_create(screens[UI_SCREEN_SETTINGS]);
    lv_label_set_text(title, "Configuracoes do Sistema");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_WARNING), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Lista de configuracoes basicas (lado esquerdo)
    lv_obj_t *config_list1 = lv_obj_create(screens[UI_SCREEN_SETTINGS]);
    lv_obj_set_size(config_list1, 220, 180);
    lv_obj_align(config_list1, LV_ALIGN_TOP_LEFT, 10, 40);
    apply_card_style(config_list1);
    
    lv_obj_t *config_title1 = lv_label_create(config_list1);
    lv_label_set_text(config_title1, "Configuracoes Basicas");
    lv_obj_set_style_text_color(config_title1, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(config_title1, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(config_title1, LV_ALIGN_TOP_LEFT, 0, 0);
    
    const char *config_items1[] = {
        "🔆 Brilho da Tela",
        "� Volume do Sistema", 
        "⏰ Data e Hora",
        "🌙 Modo Noturno"
    };
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_label_create(config_list1);
        lv_label_set_text(item, config_items1[i]);
        lv_obj_set_style_text_color(item, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
        lv_obj_set_style_text_font(item, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(item, LV_ALIGN_TOP_LEFT, 8, 25 + i * 25);
    }
    
    // Lista de configurações avançadas (lado direito)
    lv_obj_t *config_list2 = lv_obj_create(screens[UI_SCREEN_SETTINGS]);
    lv_obj_set_size(config_list2, 220, 180);
    lv_obj_align(config_list2, LV_ALIGN_TOP_RIGHT, -10, 40);
    apply_card_style(config_list2);
    
    lv_obj_t *config_title2 = lv_label_create(config_list2);
    lv_label_set_text(config_title2, "Configurações Avançadas");
    lv_obj_set_style_text_color(config_title2, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(config_title2, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(config_title2, LV_ALIGN_TOP_LEFT, 0, 0);
    
    const char *config_items2[] = {
        "� Wi-Fi / Bluetooth",
        "🔧 Calibração Sensores",
        "💾 Backup/Restore", 
        "🔄 Reset de Fábrica"
    };
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_label_create(config_list2);
        lv_label_set_text(item, config_items2[i]);
        lv_obj_set_style_text_color(item, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
        lv_obj_set_style_text_font(item, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(item, LV_ALIGN_TOP_LEFT, 8, 25 + i * 25);
    }
}

/**
 * @brief Cria a tela Sobre
 */
static void create_about_screen(void)
{
    screens[UI_SCREEN_ABOUT] = lv_obj_create(content_area);
    lv_obj_set_size(screens[UI_SCREEN_ABOUT], UI_SCREEN_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(screens[UI_SCREEN_ABOUT], LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(screens[UI_SCREEN_ABOUT], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screens[UI_SCREEN_ABOUT], UI_MARGIN_MEDIUM, LV_PART_MAIN);
    lv_obj_add_flag(screens[UI_SCREEN_ABOUT], LV_OBJ_FLAG_HIDDEN);
    
    // Título
    lv_obj_t *title = lv_label_create(screens[UI_SCREEN_ABOUT]);
    lv_label_set_text(title, "Sobre o Sistema");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    
    // Informações do sistema (lado esquerdo)
    lv_obj_t *info_card = lv_obj_create(screens[UI_SCREEN_ABOUT]);
    lv_obj_set_size(info_card, 220, 180);
    lv_obj_align(info_card, LV_ALIGN_TOP_LEFT, 10, 40);
    apply_card_style(info_card);
    
    lv_obj_t *info_title = lv_label_create(info_card);
    lv_label_set_text(info_title, "Informações do Hardware");
    lv_obj_set_style_text_color(info_title, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *info_text = lv_label_create(info_card);
    lv_label_set_text(info_text, "🖥️ ESP32-S3\\n📱 Display: 480x320\\n💾 Flash: 16MB\\n🔗 Wi-Fi/Bluetooth\\n⚡ 240MHz Dual Core");
    lv_obj_set_style_text_color(info_text, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_text, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(info_text, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // Informações do software (lado direito)
    lv_obj_t *sw_card = lv_obj_create(screens[UI_SCREEN_ABOUT]);
    lv_obj_set_size(sw_card, 220, 180);
    lv_obj_align(sw_card, LV_ALIGN_TOP_RIGHT, -10, 40);
    apply_card_style(sw_card);
    
    lv_obj_t *sw_title = lv_label_create(sw_card);
    lv_label_set_text(sw_title, "Informações do Software");
    lv_obj_set_style_text_color(sw_title, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(sw_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(sw_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *sw_text = lv_label_create(sw_card);
    lv_label_set_text(sw_text, "🚀 OS v1.0.0\\n🎨 LVGL v8.3\\n⚙️ ESP-IDF v5.1\\n📅 Build: 2024\\n👨‍💻 Embedded OS");
    lv_obj_set_style_text_color(sw_text, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(sw_text, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(sw_text, LV_ALIGN_TOP_LEFT, 0, 25);
}

// =============================================================================
// FUNÇÕES PRINCIPAIS
// =============================================================================

/**
 * @brief Inicializa o sistema operacional
 */
void ui_os_init(void)
{
    ESP_LOGI(TAG, "Inicializando Sistema Operacional ESP32-S3...");
    
    // Container principal
    main_container = lv_scr_act();
    lv_obj_set_style_bg_color(main_container, lv_color_hex(UI_COLOR_BACKGROUND), LV_PART_MAIN);
    
    // Cria componentes principais
    create_status_bar();
    create_content_area();
    create_navigation_bar();
    
    // Cria todas as telas
    create_home_screen();
    create_sensors_screen();
    create_settings_screen();
    create_about_screen();
    
    // Inicializa estado do sistema
    system_state.current_screen = UI_SCREEN_HOME;
    system_state.wifi_connected = false;
    system_state.bluetooth_enabled = false;
    system_state.battery_level = 100;
    system_state.low_battery = false;
    strcpy(system_state.time_str, "00:00");
    system_state.notification_count = 0;
    
    // Atualiza navegação para home
    ui_set_screen(UI_SCREEN_HOME);
    
    ESP_LOGI(TAG, "Sistema Operacional inicializado com sucesso!");
}

/**
 * @brief Muda para uma tela específica
 */
void ui_set_screen(ui_screen_t screen)
{
    if (screen >= UI_SCREEN_COUNT) return;
    
    // Esconde tela atual
    if (screens[current_screen]) {
        lv_obj_add_flag(screens[current_screen], LV_OBJ_FLAG_HIDDEN);
    }
    
    // Atualiza estado dos botões de navegação
    for (int i = 0; i < UI_SCREEN_COUNT; i++) {
        if (nav_buttons[i]) {
            if (i == screen) {
                lv_obj_set_style_bg_color(nav_buttons[i], lv_color_hex(UI_COLOR_PRIMARY), LV_PART_MAIN);
                lv_obj_set_style_bg_opa(nav_buttons[i], LV_OPA_30, LV_PART_MAIN);
            } else {
                lv_obj_set_style_bg_opa(nav_buttons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }
    }
    
    // Mostra nova tela
    if (screens[screen]) {
        lv_obj_clear_flag(screens[screen], LV_OBJ_FLAG_HIDDEN);
    }
    
    current_screen = screen;
    system_state.current_screen = screen;
}

/**
 * @brief Obtém a tela atual
 */
ui_screen_t ui_get_current_screen(void)
{
    return current_screen;
}

/**
 * @brief Atualiza o sistema
 */
void ui_os_update(void)
{
    // Atualiza estatísticas do sistema
    system_stats.uptime_ms = esp_timer_get_time() / 1000;
    system_stats.free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    system_stats.min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    
    // Atualiza horario (simulado)
    static uint32_t last_time_update = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_time_update > 1000) {
        uint32_t minutes = (now / 60000) % 60;
        uint32_t hours = (now / 3600000) % 24;
        snprintf(system_state.time_str, sizeof(system_state.time_str), "%02lu:%02lu", hours, minutes);
        
        if (time_label) {
            lv_label_set_text(time_label, system_state.time_str);
        }
        
        last_time_update = now;
    }
}

/**
 * @brief Atualiza dados dos sensores na interface
 */
void ui_update_sensor_data(const sensor_data_t *data)
{
    if (!data || current_screen != UI_SCREEN_SENSORS) return;
    
    // Atualiza dados do BNO055 na tela de sensores
    // Implementação específica será adicionada conforme necessário
    ESP_LOGD(TAG, "Dados de sensores atualizados");
}

/**
 * @brief Atualiza estado do sistema na interface
 */
void ui_update_system_state(const ui_system_state_t *state)
{
    if (!state) return;
    
    // Atualiza bateria
    if (battery_label) {
        static char battery_text[16];
        snprintf(battery_text, sizeof(battery_text), "%d%%", state->battery_level);
        lv_label_set_text(battery_label, battery_text);
        
        // Muda cor baseado no nível da bateria
        uint32_t color = UI_COLOR_SUCCESS;
        if (state->battery_level < 20) color = UI_COLOR_ERROR;
        else if (state->battery_level < 50) color = UI_COLOR_WARNING;
        lv_obj_set_style_text_color(battery_label, lv_color_hex(color), LV_PART_MAIN);
    }
    
    // Atualiza ícones de conectividade
    if (wifi_icon) {
        if (state->wifi_connected) {
            lv_obj_set_style_text_color(wifi_icon, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_MAIN);
            lv_obj_clear_flag(wifi_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x666666), LV_PART_MAIN);
        }
    }
    
    if (bluetooth_icon) {
        if (state->bluetooth_enabled) {
            lv_obj_set_style_text_color(bluetooth_icon, lv_color_hex(UI_COLOR_INFO), LV_PART_MAIN);
            lv_obj_clear_flag(bluetooth_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_style_text_color(bluetooth_icon, lv_color_hex(0x666666), LV_PART_MAIN);
        }
    }
    
    // Atualiza notificações
    if (notification_icon) {
        if (state->notification_count > 0) {
            lv_obj_clear_flag(notification_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(notification_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    ESP_LOGD(TAG, "Estado do sistema atualizado");
}

/**
 * @brief Atualiza estatísticas do sistema na interface
 */
void ui_update_system_stats(const ui_system_stats_t *stats)
{
    if (!stats) return;
    
    // Implementação específica será adicionada conforme necessário
    ESP_LOGD(TAG, "Estatísticas do sistema atualizadas");
}

/**
 * @brief Mostra notificação temporária
 */
void ui_show_notification(const char *message, const char *type)
{
    if (!message) return;
    
    // Remove notificação anterior se existir
    if (notification_area) {
        lv_obj_del(notification_area);
        notification_area = NULL;
    }
    
    if (notification_timer) {
        lv_timer_del(notification_timer);
        notification_timer = NULL;
    }
    
    // Cria nova notificação
    notification_area = lv_obj_create(main_container);
    lv_obj_set_size(notification_area, 280, 50);
    lv_obj_align(notification_area, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 10);
    apply_card_style(notification_area);
    
    // Define cor baseada no tipo
    uint32_t color = UI_COLOR_INFO;
    if (strcmp(type, "success") == 0) color = UI_COLOR_SUCCESS;
    else if (strcmp(type, "warning") == 0) color = UI_COLOR_WARNING;
    else if (strcmp(type, "error") == 0) color = UI_COLOR_ERROR;
    
    lv_obj_set_style_border_color(notification_area, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_border_width(notification_area, 2, LV_PART_MAIN);
    
    // Texto da notificação
    lv_obj_t *msg_label = lv_label_create(notification_area);
    lv_label_set_text(msg_label, message);
    lv_obj_set_style_text_color(msg_label, lv_color_hex(UI_COLOR_ON_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(msg_label);
    
    // Timer para esconder notificação
    notification_timer = lv_timer_create(notification_timer_cb, 3000, NULL);
    
    ESP_LOGI(TAG, "Notificação mostrada: %s (%s)", message, type);
}