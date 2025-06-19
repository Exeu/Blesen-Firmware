#include <stm32wbxx_hal.h>

#include "sensirion_arch_config.h"
#include "i2c.h"

/**
 * Execute one read transaction on the I2C bus, reading a given number of bytes.
 * If the device does not acknowledge the read command, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to read from
 * @param data    pointer to the buffer where the data is to be stored
 * @param count   number of bytes to read from I2C and store in the buffer
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
  return (int8_t)HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(address << 1),
                                        data, count, 1000);
}

/**
 * Execute one write transaction on the I2C bus, sending a given number of
 * bytes. The bytes in the supplied buffer must be sent to the given address. If
 * the slave device does not acknowledge any of the bytes, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to write to
 * @param data    pointer to the buffer containing the data to write
 * @param count   number of bytes to read from the buffer and send over I2C
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data,
                           uint16_t count) {
  return (int8_t)HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(address << 1),
                                         (uint8_t*)data, count, 100);
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_sleep_usec(uint32_t useconds) {
  uint32_t msec = useconds / 1000;
  if (useconds % 1000 > 0) {
    msec++;
  }

  /*
   * Increment by 1 if STM32F1 driver version less than 1.1.1
   * Old firmwares of STM32F1 sleep 1ms shorter than specified in HAL_Delay.
   * This was fixed with firmware 1.6 (driver version 1.1.1), so we have to
   * fix it ourselves for older firmwares
   */
  if (HAL_GetHalVersion() < 0x01010100) {
    msec++;
  }

  HAL_Delay(msec);
}
