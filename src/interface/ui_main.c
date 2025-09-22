#include "ui_main.h"
#include <esp_log.h>
#include <stdio.h>

static const char *TAG = "UI_MAIN";

// Variaveis globais para os elementos da UI
static lv_obj_t *main_screen = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *sensor_data_label = NULL;
static lv_obj_t *connection_status_label = NULL;

/**
 * @brief Atualiza o texto do label principal
 */
void ui_update_label(const char* text)
{
    if(sensor_data_label != NULL) {
        lv_label_set_text(sensor_data_label, text);
        ESP_LOGI(TAG, "Dados atualizados: %s", text);
    }
}

/**
 * @brief Atualiza status de conexao do sensor
 */
void ui_update_connection_status(bool bno055_connected)
{
    if(connection_status_label != NULL) {
        if(bno055_connected) {
            lv_label_set_text(connection_status_label, "[OK] BNO055 CONECTADO");
            lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
        } else {
            lv_label_set_text(connection_status_label, "[X] BNO055 NAO ENCONTRADO");
            lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
        }
    }
}

/**
 * @brief Cria a interface principal
 */
void ui_main_init(void)
{
    ESP_LOGI(TAG, "Inicializando interface de teste do BNO055...");
    
    // Cria a tela principal
    main_screen = lv_scr_act();
    
    // Define cor de fundo da tela
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    
    // Titulo da aplicacao
    lv_obj_t *title_label = lv_label_create(main_screen);
    lv_label_set_text(title_label, "Teste BNO055 - Debug Hardware");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status de conexao do sensor
    connection_status_label = lv_label_create(main_screen);
    lv_label_set_text(connection_status_label, "[?] VERIFICANDO BNO055...");
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(connection_status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(connection_status_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Area para dados do sensor
    sensor_data_label = lv_label_create(main_screen);
    lv_label_set_text(sensor_data_label, "Aguardando dados do sensor...");
    lv_obj_set_style_text_color(sensor_data_label, lv_color_hex(0x00FFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(sensor_data_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(sensor_data_label, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_long_mode(sensor_data_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(sensor_data_label, 300);
    
    // Status geral do sistema
    status_label = lv_label_create(main_screen);
    lv_label_set_text(status_label, "Sistema inicializando...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 80);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label, 280);
    
    // Informacoes de conexao I2C
    lv_obj_t *i2c_info = lv_label_create(main_screen);
    lv_label_set_text(i2c_info, "I2C: SDA=GPIO18, SCL=GPIO9, Addr=0x29");
    lv_obj_set_style_text_color(i2c_info, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(i2c_info, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(i2c_info, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    // Informacoes do sistema na parte inferior
    lv_obj_t *system_info = lv_label_create(main_screen);
    lv_label_set_text(system_info, "ESP32-S3 | JC3248W535EN | 320x480");
    lv_obj_set_style_text_color(system_info, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_text_font(system_info, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(system_info, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    ESP_LOGI(TAG, "Interface de teste criada com sucesso!");
}

/**
 * @brief Atualiza interface com dados dos sensores
 */
void ui_update_sensor_data(const sensor_data_t *data)
{
    if (!data) return;
    
    // Atualiza status de conexao
    if (data->bno055_valid) {
        lv_label_set_text(connection_status_label, "[OK] BNO055 CONECTADO");
        lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
        
        // Atualiza dados do sensor
        static char sensor_text[200];
        lv_snprintf(sensor_text, sizeof(sensor_text),
                   "Pitch: %.1f graus\nRoll: %.1f graus\nYaw: %.1f graus\n\nTimestamp: %lu ms",
                   data->pitch, data->roll, data->yaw, data->timestamp);
        lv_label_set_text(sensor_data_label, sensor_text);
        lv_obj_set_style_text_color(sensor_data_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
        
    } else {
        lv_label_set_text(connection_status_label, "[X] BNO055 NAO ENCONTRADO");
        lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
        
        lv_label_set_text(sensor_data_label, "Verificar conexoes:\n- SDA = GPIO18\n- SCL = GPIO9\n- VCC = 3.3V\n- GND = GND");
        lv_obj_set_style_text_color(sensor_data_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
    }
}
