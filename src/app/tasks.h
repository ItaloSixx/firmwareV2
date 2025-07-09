#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Handles das tarefas
extern TaskHandle_t ui_task_handle;
extern TaskHandle_t app_task_handle;

// Funções para inicializar e gerenciar tarefas
void app_tasks_init(void);

// Tarefa para atualização da UI
void ui_update_task(void *pvParameters);

// Tarefa principal da aplicação
void app_main_task(void *pvParameters);

// Função para parar todas as tarefas
void app_tasks_stop(void);

#endif // APP_TASKS_H
