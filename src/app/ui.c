#include "ui.h"
#include "config.h"
#include <esp_log.h>
#include <stdio.h>

static const char *TAG = APP_LOG_TAG "_UI";

// Objetos globais da UI
static lv_obj_t *main_screen = NULL;
static lv_obj_t *main_button = NULL;
static lv_obj_t *status_label = NULL;

void app_ui_init(void)
{
    ESP_LOGI(TAG, "Inicializando interface do usuário");
    app_ui_create_main_screen();
}

void app_ui_create_main_screen(void)
{
    // Criar tela principal
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x003a57), LV_PART_MAIN);
    
    // Criar título
    lv_obj_t *title = lv_label_create(main_screen);
    lv_label_set_text(title, APP_NAME " v" APP_VERSION);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Criar botão principal
    main_button = lv_btn_create(main_screen);
    lv_obj_set_size(main_button, 200, 80);
    lv_obj_center(main_button);
    lv_obj_add_event_cb(main_button, app_ui_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(main_button);
    lv_label_set_text(btn_label, "Clique Aqui");
    lv_obj_center(btn_label);
    
    // Criar label de status
    status_label = lv_label_create(main_screen);
    lv_label_set_text(status_label, "Sistema inicializado");
    lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Carregar a tela
    lv_scr_load(main_screen);
}

void app_ui_button_event_cb(lv_event_t *e)
{
    static int click_count = 0;
    click_count++;
    
    ESP_LOGI(TAG, "Botão clicado %d vezes", click_count);
    
    // Atualizar label de status
    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Botão clicado %d vezes", click_count);
    lv_label_set_text(status_label, status_text);
}

void app_ui_update(void)
{
    // Implementar atualizações periódicas da UI aqui
    // Por exemplo: atualizar valores de sensores, horário, etc.
}

void app_ui_cleanup(void)
{
    if (main_screen) {
        lv_obj_del(main_screen);
        main_screen = NULL;
    }
}
