
// #include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"
#include "main.h"
#include <esp_log.h>
#include <esp_flash.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

static const char *TAG = APP_LOG_TAG;

#define logSection(section) \
  ESP_LOGI(TAG, "\n\n************* %s **************\n", section);

/**
 * @brief Aplicação principal
 * Set the rotation degree:
 *      - 0: 0 degree
 *      - 90: 90 degree
 *      - 180: 180 degree
 *      - 270: 270 degree
 *
 */

// Removido: demos não são mais necessários para a aplicação customizada
// #include <demos/lv_demos.h>

#if !CONFIG_AUTOSTART_ARDUINO
void app_main()
{
  // initialize arduino library before we start the tasks
  // initArduino();

  setup();
}
#endif

void setup()
{
  logSection(APP_NAME " - Inicialização do Sistema");
  
  // Informações do chip
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG, "Este é um chip %s com %d núcleo(s) de CPU, %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  ESP_LOGI(TAG, "Revisão do silício v%d.%d", major_rev, minor_rev);
  
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    ESP_LOGI(TAG, "Falha ao obter tamanho da flash");
    return;
  }

  ESP_LOGI(TAG, "%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embarcada" : "externa");

  ESP_LOGI(TAG, "Tamanho mínimo de heap livre: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
  size_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  ESP_LOGI(TAG, "PSRAM livre: %d bytes", freePsram);
  
  logSection("Inicializando dispositivo de display");
  bsp_display_cfg_t cfg = {
      .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
      .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if APP_DISPLAY_ROTATION_DEGREE == 90
      .rotate = LV_DISP_ROT_90,
#elif APP_DISPLAY_ROTATION_DEGREE == 270
      .rotate = LV_DISP_ROT_270,
#elif APP_DISPLAY_ROTATION_DEGREE == 180
      .rotate = LV_DISP_ROT_180,
#elif APP_DISPLAY_ROTATION_DEGREE == 0
      .rotate = LV_DISP_ROT_NONE,
#endif
  };

  bsp_display_start_with_config(&cfg);
  bsp_display_backlight_on();

  logSection("Criando Interface do Usuário");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0);

  // Inicializar nossa UI customizada
  app_ui_init();

  /* Release the mutex */
  bsp_display_unlock();

  logSection("Inicializando Tarefas da Aplicação");
  // Inicializar tarefas
  app_tasks_init();

  logSection("Sistema Inicializado - " APP_NAME " v" APP_VERSION);
}

void loop()
{
  // Esta função não é mais utilizada no modelo baseado em tarefas
  // A lógica principal está nas tarefas criadas em app_tasks_init()
  ESP_LOGI(TAG, "Loop principal - usando modelo de tarefas FreeRTOS");
  vTaskDelay(pdMS_TO_TICKS(10000)); // Aguarda 10 segundos
}