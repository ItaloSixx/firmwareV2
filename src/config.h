/**
 * @file config.h
 * @brief Configurações centralizadas do projeto JC3248W535
 * @author Seu Nome
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURAÇÕES DE HARDWARE - JC3248W535EN
// =============================================================================

// Display (já configurado pelo BSP)
#define SCREEN_WIDTH                320
#define SCREEN_HEIGHT               480
#define LVGL_TICK_PERIOD           20

// I2C para BNO055 (usando pinos disponiveis, evitando conflito com touchscreen)
// Touchscreen usa GPIO4+8 no I2C_NUM_1, entao usamos I2C_NUM_0 com outros pinos
#define I2C_MASTER_SCL_IO          18     // GPIO18 - SCL (disponivel, nao usado pelo touch)
#define I2C_MASTER_SDA_IO          9      // GPIO9 - SDA (disponivel, nao usado pelo touch)
#define I2C_MASTER_NUM             I2C_NUM_0  // Port I2C 0 (separado do touchscreen)
#define I2C_MASTER_FREQ_HZ         100000 // 100kHz (mais compativel)

// BNO055 Reset pin (opcional mas recomendado para estabilidade)
#define BNO055_RESET_PIN           5      // GPIO5 - RST (disponivel)

// UART para LIDAR TF-Luna (pinos verificados e disponíveis)
#define LIDAR_UART_NUM             UART_NUM_1
#define LIDAR_TX_PIN               17    // GPIO17 - TX
#define LIDAR_RX_PIN               16    // GPIO16 - RX
#define LIDAR_BAUD_RATE            115200

// ADC para bateria (pino verificado e disponível)
#define BATTERY_ADC_CHANNEL        ADC1_CHANNEL_4  // GPIO5
#define BATTERY_R1                 200000.0f       // Resistor R1 do divisor
#define BATTERY_R2                 100000.0f       // Resistor R2 do divisor  
#define LOW_BATTERY_THRESHOLD      6.8f            // Tensão mínima da bateria

// Botão (pino verificado e disponível)
#define BUTTON_PIN                 14    // GPIO14

// =============================================================================
// CONFIGURAÇÕES DO BNO055
// =============================================================================

#define BNO055_I2C_ADDR            0x29  // Endereço I2C do BNO055
#define BNO055_CHIP_ID_VALUE       0xA0  // ID esperado do chip

// =============================================================================
// CONFIGURAÇÕES DE APLICAÇÃO
// =============================================================================

// Intervalos de atualização (em milissegundos)
#define SENSOR_UPDATE_INTERVAL     500   // Atualiza sensores a cada 500ms
#define UI_UPDATE_INTERVAL         100   // Atualiza UI a cada 100ms
#define BATTERY_CHECK_INTERVAL     5000  // Verifica bateria a cada 5s

// Configurações de calibração
#define CALIB_COUNTDOWN_TIME       3000  // 3 segundos para cada passo
#define CALIB_SAMPLES              10    // Número de amostras para calibração

// =============================================================================
// CONFIGURAÇÕES DE DEBUG
// =============================================================================

#define ENABLE_SENSOR_DEBUG        1     // 1 = habilita debug dos sensores
#define ENABLE_UI_DEBUG            0     // 1 = habilita debug da UI
#define ENABLE_CALIB_DEBUG         1     // 1 = habilita debug da calibração

// =============================================================================
// ESTADOS DA APLICAÇÃO
// =============================================================================

typedef enum {
    APP_STATE_INIT,
    APP_STATE_MAIN_MENU,
    APP_STATE_MEASURING,
    APP_STATE_CALIBRATION,
    APP_STATE_ERROR
} app_state_t;

typedef enum {
    MEASUREMENT_IDLE,
    MEASUREMENT_CENTER,
    MEASUREMENT_BASE,
    MEASUREMENT_TOP,
    MEASUREMENT_COMPLETE
} measurement_state_t;

typedef enum {
    CALIB_IDLE,
    CALIB_ZERO_REFERENCE,
    CALIB_VERIFY_ANGLES,
    CALIB_SAVE_PROFILE,
    CALIB_COMPLETE
} calibration_step_t;

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
