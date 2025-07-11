#ifndef BLESEN_FW_STM32_WPAN_APP_ADC_SENSORS_H_
#define BLESEN_FW_STM32_WPAN_APP_ADC_SENSORS_H_

#include <stdint.h>

#define ENABLE_ALTERNATE_LUX_FORMULA  1

typedef struct {
    uint16_t RawAdcValues[2];
    uint32_t VRefInt, Brightness, VBat, MCUTemperature;
    uint8_t BatteryPercent;
    int32_t Temperature;
    int32_t Humidity;
} sensor_data_t;

void read_sensors(sensor_data_t *sen_data);

void read_i2c_sensor(sensor_data_t *sen_data);

#endif
