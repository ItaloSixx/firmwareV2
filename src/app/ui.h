#ifndef APP_UI_H
#define APP_UI_H

#include <lvgl.h>

// Função para inicializar a interface do usuário
void app_ui_init(void);

// Função para criar a tela principal
void app_ui_create_main_screen(void);

// Callbacks de eventos
void app_ui_button_event_cb(lv_event_t *e);

// Função para atualizar a UI
void app_ui_update(void);

// Função para limpar recursos da UI
void app_ui_cleanup(void);

#endif // APP_UI_H
