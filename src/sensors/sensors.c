#include "sensors.h"
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
static bool bno055_initialized = false;
static sensor_data_t last_reading = {0};

/**
 * @brief Inicializa o pino de reset do BNO055
 */
static esp_err_t bno055_reset_pin_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << BNO055_RESET_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    return gpio_config(&io_conf);
}

/**
 * @brief Faz reset fisico do BNO055 via GPIO
 */
static void bno055_hardware_reset(void)
{
    ESP_LOGI(TAG, "Fazendo reset fisico do BNO055 via GPIO%d...", BNO055_RESET_PIN);
    
    // Reset baixo por 10ms
    gpio_set_level(BNO055_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Reset alto - libera o chip
    gpio_set_level(BNO055_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(650)); // Aguarda 650ms para boot completo
    
    ESP_LOGI(TAG, "Reset fisico do BNO055 concluido");
}

/**
 * @brief Escreve dados no BNO055 via I2C
 */
static esp_err_t bno055_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, BNO055_I2C_ADDRESS, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

/**
 * @brief Le dados do BNO055 via I2C
 */
static esp_err_t bno055_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, BNO055_I2C_ADDRESS, &reg, 1, data, len, pdMS_TO_TICKS(1000));
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
        ESP_LOGW(TAG, "Verificar:");
        ESP_LOGW(TAG, "- Conexoes fisicas SDA=%d, SCL=%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
        ESP_LOGW(TAG, "- Alimentacao 3.3V do BNO055");
        ESP_LOGW(TAG, "- Pull-ups nos pinos I2C (4.7k ohm)");
        ESP_LOGW(TAG, "- Soldas/jumpers");
    } else {
        ESP_LOGI(TAG, "Total de dispositivos encontrados: %d", devices_found);
    }
    ESP_LOGI(TAG, "=== Fim do Scanner I2C ===");
}

/**
 * @brief Inicializa o I2C master com configuração específica para BNO055
 */
static esp_err_t i2c_master_init(void)
{
    ESP_LOGI(TAG, "Configurando I2C para BNO055...");
    ESP_LOGI(TAG, "SDA: GPIO%d, SCL: GPIO%d, Port: I2C_NUM_%d, Freq: %d Hz", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_NUM, I2C_MASTER_FREQ_HZ);
    
    // Para o driver I2C se já estiver instalado (evita conflitos)
    esp_err_t ret = i2c_driver_delete(I2C_MASTER_NUM);
    if (ret == ESP_OK) {
        ESP_LOGW(TAG, "Driver I2C anterior removido do port %d", I2C_MASTER_NUM);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO,
        .scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ
        },
        .clk_flags = 0,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao configurar parametros I2C: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao instalar driver I2C: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C inicializado com sucesso no port I2C_NUM_%d", I2C_MASTER_NUM);
    return ESP_OK;
}

/**
 * @brief Reinicializa o I2C em caso de problemas
 */
static esp_err_t i2c_reinit(void)
{
    ESP_LOGW(TAG, "Reinicializando I2C por problemas de comunicacao...");
    
    // Remove driver atual
    i2c_driver_delete(I2C_MASTER_NUM);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reinicializa
    return i2c_master_init();
}

/**
 * @brief Testa comunicacao com BNO055 com retry
 */
static bool bno055_test_communication(int max_retries)
{
    for (int retry = 0; retry < max_retries; retry++) {
        ESP_LOGI(TAG, "Tentativa %d/%d de comunicacao com BNO055...", retry + 1, max_retries);
        
        uint8_t chip_id;
        esp_err_t ret = bno055_read_reg(BNO055_CHIP_ID_REG, &chip_id, 1);
        
        if (ret == ESP_OK && chip_id == BNO055_CHIP_ID_VALUE) {
            ESP_LOGI(TAG, "*** BNO055 respondeu! Chip ID: 0x%02X ***", chip_id);
            return true;
        }
        
        ESP_LOGW(TAG, "Falha na tentativa %d (ret=%s, id=0x%02X)", retry + 1, esp_err_to_name(ret), chip_id);
        
        if (retry < max_retries - 1) {
            ESP_LOGI(TAG, "Aguardando antes da proxima tentativa...");
            vTaskDelay(pdMS_TO_TICKS(500));
            
            // A cada 2 tentativas, reinicializa I2C
            if ((retry + 1) % 2 == 0) {
                ESP_LOGI(TAG, "Reinicializando I2C...");
                i2c_reinit();
            }
        }
    }
    
    return false;
}   

/**
 * @brief Escaneia o barramento I2C para encontrar dispositivos
 */
static void i2c_scan_devices(void)
{
    ESP_LOGI(TAG, "Escaneando barramento I2C...");
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Dispositivo I2C encontrado no endereco: 0x%02X", addr);
        }
    }
    
    ESP_LOGI(TAG, "Scan I2C completo.");
}

/**
 * @brief Inicializa o BNO055
 */
bool bno055_init(void)
{
    ESP_LOGI(TAG, "Inicializando BNO055...");
    ESP_LOGI(TAG, "I2C Config: SDA=GPIO%d, SCL=GPIO%d, Port=%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_NUM);
    
    // Inicializa pino de reset
    if (bno055_reset_pin_init() != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar pino de reset GPIO%d", BNO055_RESET_PIN);
        return false;
    }
    
    // Faz reset fisico primeiro
    bno055_hardware_reset();
    
    // Primeiro faz scan do barramento I2C
    i2c_scan_devices();
    
    // Testa comunicacao com retry
    if (!bno055_test_communication(5)) {
        ESP_LOGE(TAG, "BNO055 nao responde apos multiplas tentativas!");
        ESP_LOGE(TAG, "Verificar:");
        ESP_LOGE(TAG, "- Conexoes fisicas (SDA=GPIO%d, SCL=GPIO%d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
        ESP_LOGE(TAG, "- Pino RESET conectado ao GPIO%d", BNO055_RESET_PIN);
        ESP_LOGE(TAG, "- Alimentacao 3.3V estavel");
        ESP_LOGE(TAG, "- Endereco I2C 0x%02X", BNO055_I2C_ADDRESS);
        ESP_LOGE(TAG, "- Modulo BNO055 funcionando");
        return false;
    }
    
    // Reset do sistema
    bno055_write_reg(BNO055_SYS_TRIGGER_REG, 0x20);
    vTaskDelay(pdMS_TO_TICKS(700)); // Aguarda reset

    // Verifica se ainda responde apos reset
    esp_err_t ret;
    uint8_t chip_id;
    ret = bno055_read_reg(BNO055_CHIP_ID_REG, &chip_id, 1);
    if (ret != ESP_OK || chip_id != BNO055_CHIP_ID_VALUE) {
        ESP_LOGE(TAG, "BNO055 nao responde apos reset!");
        return false;
    }
    
    // Configura modo de operacao
    bno055_write_reg(BNO055_OPR_MODE_REG, BNO055_OPERATION_MODE_CONFIG);
    vTaskDelay(pdMS_TO_TICKS(25));
    
    // Configura modo NDOF (Nine Degrees of Freedom)
    bno055_write_reg(BNO055_OPR_MODE_REG, BNO055_OPERATION_MODE_NDOF);
    vTaskDelay(pdMS_TO_TICKS(20));
    
    bno055_initialized = true;
    ESP_LOGI(TAG, "BNO055 inicializado em modo NDOF");
    
    return true;
}

/**
 * @brief Le dados de orientacao do BNO055
 */
bool bno055_read_euler(float *pitch, float *roll, float *yaw)
{
    if (!bno055_initialized) {
        return false;
    }
    
    uint8_t euler_data[6];
    esp_err_t ret = bno055_read_reg(BNO055_EULER_H_LSB_REG, euler_data, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler dados do BNO055");
        return false;
    }
    
    // Converte os dados (LSB, MSB) para valores de ponto flutuante
    int16_t yaw_raw = (int16_t)((euler_data[1] << 8) | euler_data[0]);
    int16_t roll_raw = (int16_t)((euler_data[3] << 8) | euler_data[2]);
    int16_t pitch_raw = (int16_t)((euler_data[5] << 8) | euler_data[4]);
    
    // Converte para graus (resolucao de 1/16 grau)
    *yaw = yaw_raw / 16.0f;
    *roll = roll_raw / 16.0f;
    *pitch = pitch_raw / 16.0f;
    
    return true;
}

/**
 * @brief Obtem status de calibracao do BNO055
 */
void bno055_get_calibration_status(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag)
{
    if (!bno055_initialized) {
        *sys = *gyro = *accel = *mag = 0;
        return;
    }
    
    uint8_t calib_data;
    esp_err_t ret = bno055_read_reg(BNO055_CALIB_STAT_REG, &calib_data, 1);
    if (ret != ESP_OK) {
        *sys = *gyro = *accel = *mag = 0;
        return;
    }
    
    *mag = (calib_data >> 0) & 0x03;
    *accel = (calib_data >> 2) & 0x03;
    *gyro = (calib_data >> 4) & 0x03;
    *sys = (calib_data >> 6) & 0x03;
}

/**
 * @brief Inicializa todos os sensores
 */
bool sensors_init(void)
{
    ESP_LOGI(TAG, "Inicializando sistema de sensores (BNO055 apenas)...");
    
    // Inicializa I2C
    if (i2c_master_init() != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao inicializar I2C");
        return false;
    }
    
    ESP_LOGI(TAG, "I2C inicializado. Executando scanner para debug...");
    
    // Executa scanner I2C para debug
    i2c_scanner();
    
    // Aguarda um pouco antes de tentar inicializar BNO055
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Inicializa BNO055
    if (!bno055_init()) {
        ESP_LOGE(TAG, "BNO055 nao inicializado - verificar conexoes");
        // Continua mesmo sem BNO055 para permitir debug
    }
    
    // Inicializa valores padrao
    last_reading.lidar_distance = 0;
    last_reading.pitch = 0.0f;
    last_reading.roll = 0.0f;
    last_reading.yaw = 0.0f;
    last_reading.roll_offset = 0.0f;
    last_reading.battery_voltage = 0.0f;
    last_reading.bno055_valid = bno055_initialized;
    last_reading.lidar_valid = false; // LIDAR desabilitado
    last_reading.low_battery = false;
    last_reading.timestamp = esp_log_timestamp();
    
    sensors_initialized = true;
    ESP_LOGI(TAG, "Sistema de sensores inicializado (BNO055: %s)!", 
             bno055_initialized ? "OK" : "FALHOU");
    
    return true;
}

/**
 * @brief Le todos os sensores
 */
bool sensors_read_all(sensor_data_t *data)
{
    if (!sensors_initialized || data == NULL) {
        return false;
    }
    
    // Le BNO055
    if (bno055_initialized) {
        data->bno055_valid = bno055_read_euler(&data->pitch, &data->roll, &data->yaw);
        if (!data->bno055_valid) {
            ESP_LOGW(TAG, "Falha ao ler BNO055 - dados antigos mantidos");
        }
    } else {
        data->bno055_valid = false;
        data->pitch = 0.0f;
        data->roll = 0.0f;
        data->yaw = 0.0f;
    }
    
    // LIDAR desabilitado por enquanto
    data->lidar_distance = 0;
    data->lidar_valid = false;
    
    // Bateria desabilitada por enquanto
    data->battery_voltage = 0.0f;
    data->low_battery = false;
    
    data->timestamp = esp_log_timestamp();
    
    // Atualiza cache interno
    last_reading = *data;
    
    if (data->bno055_valid) {
        ESP_LOGI(TAG, "BNO055 - Pitch: %.1f°, Roll: %.1f°, Yaw: %.1f°",
                 data->pitch, data->roll, data->yaw);
    } else {
        ESP_LOGD(TAG, "BNO055 nao disponivel");
    }
    
    return true;
}

/**
 * @brief Deinicializa os sensores
 */
void sensors_deinit(void)
{
    ESP_LOGI(TAG, "Desinicializando sensores...");
    
    i2c_driver_delete(I2C_MASTER_NUM);
    
    bno055_initialized = false;
    sensors_initialized = false;
    
    ESP_LOGI(TAG, "Sensores desinicializados!");
}
