#include "sht4x.h"
#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

/* all measurement commands return T (CRC) RH (CRC) */
#define SHT4X_CMD_MEASURE_HPM 0xFD
#define SHT4X_CMD_MEASURE_LPM 0xE0
#define SHT4X_CMD_READ_SERIAL 0x89
#define SHT4X_CMD_DURATION_USEC 1000

#define SHT4X_ADDRESS 0x44

static uint8_t sht4x_cmd_measure = SHT4X_CMD_MEASURE_HPM;
static uint16_t sht4x_cmd_measure_delay_us = SHT4X_MEASUREMENT_DURATION_USEC;

int16_t sht4x_measure_blocking_read(int32_t* temperature, uint32_t* humidity) {
    int16_t ret;

    ret = sht4x_measure();
    if (ret)
        return ret;
    sensirion_sleep_usec(sht4x_cmd_measure_delay_us);
    return sht4x_read(temperature, humidity);
}

int16_t sht4x_measure(void) {
    return sensirion_i2c_write(SHT4X_ADDRESS, &sht4x_cmd_measure, 1);
}

int16_t sht4x_read(int32_t* temperature, int32_t* humidity) {
    uint16_t words[2];
    int16_t ret = sensirion_i2c_read_words(SHT4X_ADDRESS, words,SENSIRION_NUM_WORDS(words));
    /**
     * formulas for conversion of the sensor signals, optimized for fixed point
     * algebra:
     * Temperature = 175 * S_T / 65535 - 45
     * Relative Humidity = 125 * (S_RH / 65535) - 6
     */
    *temperature = ((21875 * (int32_t)words[0]) >> 13) - 45000;
    *humidity = (((int32_t)words[1] * 125) / 65535) - 6;

    return ret;
}


int16_t sht4x_probe(void) {
    uint32_t serial;

    return sht4x_read_serial(&serial);
}


int16_t sht4x_read_serial(uint32_t* serial) {
    const uint8_t cmd = SHT4X_CMD_READ_SERIAL;
    int16_t ret;
    uint16_t serial_words[SENSIRION_NUM_WORDS(*serial)];

    ret = sensirion_i2c_write(SHT4X_ADDRESS, &cmd, 1);
    if (ret)
        return ret;

    sensirion_sleep_usec(SHT4X_CMD_DURATION_USEC);
    ret = sensirion_i2c_read_words(SHT4X_ADDRESS, serial_words,
                                   SENSIRION_NUM_WORDS(serial_words));
    *serial = ((uint32_t)serial_words[0] << 16) | serial_words[1];

    return ret;
}
