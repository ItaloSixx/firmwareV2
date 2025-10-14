#include "sensors.h"
#include "../config.h"
#include <esp_log.h>
#include <esp_system.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "SENSORS";

// Variaveis internas
static bool sensors_initialized = false;
static sensor_data_t last_reading = {0};

#if ENABLE_BNO055
static bool bno055_initialized = false;

/**
 * @brief Inicializa o pino de reset do BNO055
 */
static esp_err_t bno055_reset_pin_init(void)
{
    // BNO055 temporariamente desabilitado
    return ESP_OK;
}

/**
 * @brief Faz reset fisico do BNO055 via GPIO (DESABILITADO)
 */
static void bno055_hardware_reset(void)
{
    // BNO055 temporariamente desabilitado
    ESP_LOGI(TAG, "BNO055 reset desabilitado temporariamente");
}

/**
 * @brief Escreve dados no BNO055 via I2C (DESABILITADO)
 */
static esp_err_t bno055_write_reg(uint8_t reg, uint8_t data)
{
    // BNO055 temporariamente desabilitado
    (void)reg;
    (void)data;
    return ESP_OK;
}

/**
 * @brief Le dados do BNO055 via I2C (DESABILITADO)
 */
static esp_err_t bno055_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    // BNO055 temporariamente desabilitado
    (void)reg;
    if (data && len > 0) {
        memset(data, 0, len);
    }
    return ESP_OK;
}

/**
 * @brief Scanner I2C para debug - encontra todos os dispositivos
 */
static void i2c_scanner(void)
{
    ESP_LOGI(TAG, "=== Scanner I2C ===");
    ESP_LOGI(TAG, "Escaneando enderecos I2C de 0x08 a 0x77...");
    ESP_LOGI(TAG, "Configuracao: SDA=GPIO%d, SCL=GPIO%d, Porta=%d", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_NUM);
    
    int devices_found = 0;
    
    for (uint8_t address = 0x08; address <= 0x77; address++) {
        // Cria comando I2C vazio para testar dispositivo
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Dispositivo I2C encontrado no endereco 0x%02X", address);
            devices_found++;
            
            // Se for BNO055, verifica chip ID
            if (address == BNO055_I2C_ADDRESS) {
                ESP_LOGI(TAG, "*** Encontrado BNO055 no endereco correto! ***");
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGD(TAG, "Timeout no endereco 0x%02X", address);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1)); // Pequeno delay entre scans
    }
    
    if (devices_found == 0) {
        ESP_LOGW(TAG, "Nenhum dispositivo I2C encontrado!");
        ESP_LOGW(TAG, "BNO055 temporariamente desabilitado");
    } else {
        ESP_LOGI(TAG, "Total de dispositivos encontrados: %d", devices_found);
    }
    ESP_LOGI(TAG, "=== Fim do Scanner I2C ===");
}

/**
 * @brief Inicializa o I2C master (DESABILITADO para BNO055)
 */
static esp_err_t i2c_master_init(void)
{
    ESP_LOGI(TAG, "Configurando I2C para BNO055...");
    // BNO055 I2C temporariamente desabilitado
    ESP_LOGI(TAG, "I2C para BNO055 desabilitado temporariamente");
    return ESP_OK;
}

/**
 * @brief Reinicializa o I2C (DESABILITADO)
 */
static esp_err_t i2c_reinit(void)
{
    // BNO055 temporariamente desabilitado
    ESP_LOGW(TAG, "I2C reinit desabilitado (BNO055 off)");
    return ESP_OK;
}

/**
 * @brief Testa comunicacao com BNO055 (DESABILITADO)
 */
static bool bno055_test_communication(int max_retries)
{
    // BNO055 temporariamente desabilitado
    (void)max_retries;
    ESP_LOGI(TAG, "BNO055 test desabilitado temporariamente");
    return false;
}   

/**
 * @brief Escaneia o barramento I2C (DESABILITADO)
 */
static void i2c_scan_devices(void)
{
    // BNO055 temporariamente desabilitado
    ESP_LOGI(TAG, "I2C scan desabilitado (BNO055 off)");
}

/**
 * @brief Inicializa o BNO055
 */
bool bno055_init(void)
{
    ESP_LOGI(TAG, "BNO055 inicialização desabilitada temporariamente");
    return true; // Simula sucesso para não quebrar o sistema
}

/**
 * @brief Le dados de orientacao do BNO055 (DESABILITADO)
 */
bool bno055_read_euler(float *pitch, float *roll, float *yaw)
{
    // BNO055 temporariamente desabilitado
    if (pitch) *pitch = 0.0f;
    if (roll) *roll = 0.0f; 
    if (yaw) *yaw = 0.0f;
    return false;
}

/**
 * @brief Obtem status de calibracao do BNO055 (DESABILITADO)
 */
void bno055_get_calibration_status(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag)
{
    // BNO055 temporariamente desabilitado
    if (sys) *sys = 0;
    if (gyro) *gyro = 0;
    if (accel) *accel = 0;
    if (mag) *mag = 0;
}

#endif // ENABLE_BNO055

/**
 * @brief Inicializa todos os sensores
 */
bool sensors_init(void)
{
    ESP_LOGI(TAG, "Inicializando sistema de sensores (BNO055 desabilitado)...");
    
    // Inicializa valores padrão
    last_reading.lidar_distance = 0;
    last_reading.pitch = 0.0f;
    last_reading.roll = 0.0f;
    last_reading.yaw = 0.0f;
    last_reading.roll_offset = 0.0f;
    last_reading.battery_voltage = 0.0f;
    last_reading.bno055_valid = false; // BNO055 desabilitado
    last_reading.lidar_valid = false;
    last_reading.low_battery = false;
    last_reading.timestamp = esp_log_timestamp();
    
    sensors_initialized = true;
    ESP_LOGI(TAG, "Sistema de sensores inicializado (BNO055 desabilitado temporariamente)");
    
    return true;
}

/**
 * @brief Le todos os sensores
 */
bool sensors_read_all(sensor_data_t *data_out)
{
    if (!sensors_initialized || data_out == NULL) {
        return false;
    }
    
    // BNO055 desabilitado - retorna valores zero
    data_out->bno055_valid = false;
    data_out->pitch = 0.0f;
    data_out->roll = 0.0f;
    data_out->yaw = 0.0f;
    
    // LIDAR desabilitado temporariamente
    data_out->lidar_distance = 0;
    data_out->lidar_valid = false;
    
    // Bateria desabilitada temporariamente
    data_out->battery_voltage = 0.0f;
    data_out->low_battery = false;
    
    // Atualizar timestamp
    data_out->timestamp = esp_log_timestamp();
    
    // Salvar última leitura
    last_reading = *data_out;
    
    return true;
}

/**
 * @brief Deinicializa os sensores
 */
void sensors_deinit(void)
{
    ESP_LOGI(TAG, "Desinicializando sensores...");
    
    sensors_initialized = false;
    
    ESP_LOGI(TAG, "Sensores desinicializados!");
}
