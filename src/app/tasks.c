#include "tasks.h"
#include "ui.h"
#include "config.h"
#include <esp_log.h>

static const char *TAG = APP_LOG_TAG "_TASKS";

// Handles das tarefas
TaskHandle_t ui_task_handle = NULL;
TaskHandle_t app_task_handle = NULL;

void app_tasks_init(void)
{
    ESP_LOGI(TAG, "Inicializando tarefas da aplicação");
    
    // Criar tarefa de atualização da UI
    xTaskCreate(
        ui_update_task,
        "ui_update",
        APP_TASK_STACK_SIZE,
        NULL,
        APP_TASK_PRIORITY,
        &ui_task_handle
    );
    
    // Criar tarefa principal da aplicação
    xTaskCreate(
        app_main_task,
        "app_main",
        APP_TASK_STACK_SIZE,
        NULL,
        APP_TASK_PRIORITY + 1,
        &app_task_handle
    );
    
    ESP_LOGI(TAG, "Tarefas criadas com sucesso");
}

void ui_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Tarefa de atualização da UI iniciada");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(APP_UI_UPDATE_PERIOD_MS);
    
    while (1) {
        // Atualizar UI
        app_ui_update();
        
        // Aguardar próximo ciclo
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void app_main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Tarefa principal da aplicação iniciada");
    
    while (1) {
        // Implementar lógica principal da aplicação aqui
        // Por exemplo: leitura de sensores, processamento de dados, etc.
        
        ESP_LOGI(TAG, "Executando ciclo principal da aplicação");
        
        // Aguardar 5 segundos antes do próximo ciclo
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_tasks_stop(void)
{
    ESP_LOGI(TAG, "Parando tarefas da aplicação");
    
    if (ui_task_handle) {
        vTaskDelete(ui_task_handle);
        ui_task_handle = NULL;
    }
    
    if (app_task_handle) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
    }
}
