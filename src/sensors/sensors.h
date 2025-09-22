#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Definicoes do BNO055
#define BNO055_I2C_ADDRESS          0x29
#define BNO055_CHIP_ID_VALUE        0xA0

// Registradores importantes do BNO055
#define BNO055_CHIP_ID_REG          0x00
#define BNO055_ACC_ID_REG           0x01
#define BNO055_MAG_ID_REG           0x02
#define BNO055_GYR_ID_REG           0x03
#define BNO055_SW_REV_ID_LSB_REG    0x04
#define BNO055_SW_REV_ID_MSB_REG    0x05
#define BNO055_BL_REV_ID_REG        0x06

// Registradores de dados Euler
#define BNO055_EULER_H_LSB_REG      0x1A
#define BNO055_EULER_H_MSB_REG      0x1B
#define BNO055_EULER_R_LSB_REG      0x1C
#define BNO055_EULER_R_MSB_REG      0x1D
#define BNO055_EULER_P_LSB_REG      0x1E
#define BNO055_EULER_P_MSB_REG      0x1F

// Registradores de configuracao
#define BNO055_OPR_MODE_REG         0x3D
#define BNO055_PWR_MODE_REG         0x3E
#define BNO055_SYS_TRIGGER_REG      0x3F
#define BNO055_TEMP_SOURCE_REG      0x40
#define BNO055_AXIS_MAP_CONFIG_REG  0x41
#define BNO055_AXIS_MAP_SIGN_REG    0x42

// Registradores de calibracao
#define BNO055_CALIB_STAT_REG       0x35
#define BNO055_SYS_STAT_REG         0x39
#define BNO055_SYS_ERR_REG          0x3A

// Modos de operacao
#define BNO055_OPERATION_MODE_CONFIG    0x00
#define BNO055_OPERATION_MODE_ACCONLY   0x01
#define BNO055_OPERATION_MODE_MAGONLY   0x02
#define BNO055_OPERATION_MODE_GYRONLY   0x03
#define BNO055_OPERATION_MODE_ACCMAG    0x04
#define BNO055_OPERATION_MODE_ACCGYRO   0x05
#define BNO055_OPERATION_MODE_MAGGYRO   0x06
#define BNO055_OPERATION_MODE_AMG       0x07
#define BNO055_OPERATION_MODE_IMUPLUS   0x08
#define BNO055_OPERATION_MODE_COMPASS   0x09
#define BNO055_OPERATION_MODE_M4G       0x0A
#define BNO055_OPERATION_MODE_NDOF_FMC_OFF 0x0B
#define BNO055_OPERATION_MODE_NDOF      0x0C

// Estrutura de dados dos sensores
typedef struct {
    // BNO055
    float pitch;
    float roll;
    float yaw;
    float roll_offset;
    bool bno055_valid;
    
    // LIDAR (para futuro)
    int lidar_distance;
    bool lidar_valid;
    
    // Bateria (para futuro)
    float battery_voltage;
    bool low_battery;
    
    // Timestamp
    uint32_t timestamp;
} sensor_data_t;

// Funcoes publicas
bool sensors_init(void);
bool sensors_read_all(sensor_data_t *data);
void sensors_deinit(void);

// Funcoes especificas do BNO055
bool bno055_init(void);
bool bno055_read_euler(float *pitch, float *roll, float *yaw);
void bno055_get_calibration_status(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag);

#endif // SENSORS_H
