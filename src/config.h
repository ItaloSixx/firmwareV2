/**
 * @file config.h
 * @brie// UART para LIDAR TF-Plus (usando GPIO livres conforme solicitado)
#define LIDAR_UART_NUM             UART_NUM_1
#define LIDAR_TX_PIN               17    // GPIO17 - TX do ESP32 -> RX do LiDAR
#define LIDAR_RX_PIN               18    // GPIO18 - RX do ESP32 <- TX do LiDAR
#define LIDAR_BAUD_RATE            115200igurações centralizadas do projeto JC3248W535
 * @author Seu Nome
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURACOES DE HARDWARE - JC3248W535EN
// =============================================================================

// Display (configuracao para modo horizontal/landscape)
#define LVGL_PORT_ROTATION_DEGREE   90
#define SCREEN_WIDTH                480
#define SCREEN_HEIGHT               320
#define LVGL_TICK_PERIOD           20

// I2C para BNO055 (TEMPORARIAMENTE DESABILITADO)
// Touchscreen usa GPIO4(SCL)+8(SDA) no I2C_NUM_1, entao usamos I2C_NUM_0 com outros pinos
// #define I2C_MASTER_SCL_IO          18     // GPIO18 - SCL (livre conforme esquematico)
// #define I2C_MASTER_SDA_IO          17     // GPIO17 - SDA (livre conforme esquematico)  
// #define I2C_MASTER_NUM             I2C_NUM_0  // Port I2C 0 (separado do touchscreen)
// #define I2C_MASTER_FREQ_HZ         100000 // 100kHz (mais compativel)
// #define BNO055_RESET_PIN           6      // GPIO6 - Reset do BNO055 (DESABILITADO)

// UART para LIDAR TF-Plus (conforme esquematico JC3248W535EN)
// UART1: GPIO43=TX1, GPIO44=RX1 (pinos dedicados do ESP32-S3)
#define LIDAR_UART_NUM             UART_NUM_1
#define LIDAR_TX_PIN               17    // GPIO17 - TX1 do ESP32 -> RX do LiDAR
#define LIDAR_RX_PIN               18    // GPIO18 - RX1 do ESP32 <- TX do LiDAR
#define LIDAR_BAUD_RATE            115200

// ADC para bateria (conforme esquematico - usando GPIO livre)
#define BATTERY_ADC_CHANNEL        ADC1_CHANNEL_6  // GPIO7 - livre conforme esquematico
#define BATTERY_R1                 200000.0f       // Resistor R1 do divisor
#define BATTERY_R2                 100000.0f       // Resistor R2 do divisor  
#define LOW_BATTERY_THRESHOLD      6.8f            // Tensão mínima da bateria

// Botão de medição física (conforme esquematico JC3248W535EN)
#define MEASUREMENT_BUTTON_PIN     5     // GPIO5 - livre, ideal para botão físico
#define BUTTON_PIN                 14    // GPIO14 - GPIO livre adicional (reserva)

// Pinos livres adicionais conforme esquematico:
// GPIO6, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO15, GPIO16

// Configurações do botão físico
#define BUTTON_DEBOUNCE_MS         50   // Debounce de 50ms para evitar ruído
#define BUTTON_LONG_PRESS_MS       1000 // Pressão longa = 1 segundo

// =============================================================================
// CONFIGURAÇÕES DO BNO055 (TEMPORARIAMENTE DESABILITADO)
// =============================================================================

// Pino de reset do BNO055 (temporariamente não usado)
#define BNO055_RESET_PIN           16    // GPIO16 - pino livre para reset

// Configurações I2C do BNO055 (temporariamente desabilitado)
#define BNO055_I2C_ADDR            0x29  // Endereço I2C do BNO055
#define BNO055_CHIP_ID_VALUE       0xA0  // ID esperado do chip

// Flag para desabilitar BNO055 temporariamente
#define ENABLE_BNO055              0     // 0 = desabilitado, 1 = habilitado

// =============================================================================
// CONFIGURAÇÕES DE APLICAÇÃO
// =============================================================================

// Intervalos de atualização (em milissegundos)
#define SENSOR_UPDATE_INTERVAL     500   // Atualiza sensores a cada 500ms
#define UI_UPDATE_INTERVAL         100   // Atualiza UI a cada 100ms
#define BATTERY_CHECK_INTERVAL     5000  // Verifica bateria a cada 5s
#define LIDAR_UPDATE_INTERVAL      200   // Atualiza LiDAR a cada 200ms

// Configurações do sistema de medição
#define MAX_MEASUREMENT_WIDGETS    10    // Máximo de widgets de medição
#define WIDGET_HEIGHT              40    // Altura padrão dos widgets
#define WIDGET_MARGIN              10    // Margem entre widgets
#define MEASUREMENT_TIMEOUT_MS     30000 // Timeout para medições (30s)

// Configurações de calibração
#define CALIB_COUNTDOWN_TIME       3000  // 3 segundos para cada passo
#define CALIB_SAMPLES              10    // Número de amostras para calibração

// =============================================================================
// CONFIGURAÇÕES DE DEBUG
// =============================================================================

#define ENABLE_SENSOR_DEBUG        1     // 1 = habilita debug dos sensores
#define ENABLE_UI_DEBUG            0     // 1 = habilita debug da UI
#define ENABLE_CALIB_DEBUG         1     // 1 = habilita debug da calibração
#define ENABLE_BUTTON_DEBUG        1     // 1 = habilita debug do botão físico
#define ENABLE_LIDAR_DEBUG         1     // 1 = habilita debug do LiDAR

// =============================================================================
// CONFIGURAÇÕES DOS WIDGETS DE MEDIÇÃO
// =============================================================================

// Cores dos widgets (Material Design)
#define WIDGET_COLOR_PRIMARY       0x2196F3  // Azul Material
#define WIDGET_COLOR_SUCCESS       0x4CAF50  // Verde Material
#define WIDGET_COLOR_WARNING       0xFF9800  // Laranja Material
#define WIDGET_COLOR_ERROR         0xF44336  // Vermelho Material
#define WIDGET_COLOR_TEXT          0x212121  // Texto escuro
#define WIDGET_COLOR_BACKGROUND    0xFAFAFA  // Fundo claro

// Configurações de layout dos widgets
#define MEASUREMENT_CONTAINER_WIDTH   460    // Largura do container
#define MEASUREMENT_CONTAINER_HEIGHT  280    // Altura do container
#define WIDGET_PADDING               8       // Padding interno dos widgets
#define WIDGET_BORDER_RADIUS         8       // Raio das bordas arredondadas

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

// Estados específicos para medição com botão físico
typedef enum {
    MEASUREMENT_IDLE,           // Aguardando início
    MEASUREMENT_HORIZONTAL,     // Medindo distância horizontal
    MEASUREMENT_TOP,           // Medindo distância até o topo
    MEASUREMENT_BASE,          // Medindo distância até a base  
    MEASUREMENT_CALCULATING,   // Calculando alturas
    MEASUREMENT_COMPLETE,      // Medição completa
    MEASUREMENT_SAVING        // Salvando dados
} measurement_state_t;

// Estados do botão físico
typedef enum {
    BUTTON_STATE_RELEASED,     // Botão solto
    BUTTON_STATE_PRESSED,      // Botão pressionado
    BUTTON_STATE_DEBOUNCING    // Em processo de debounce
} button_state_t;

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
