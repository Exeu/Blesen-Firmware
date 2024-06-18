#include "blesen-service.h"
#include "app_conf.h"
#include "ble_gap_aci.h"
#include "adc-sensors.h"
#include "rtc.h"


uint8_t service_data[SERVICE_DATA_LENGTH] = {
    sizeof(service_data) - 1,
    AD_TYPE_SERVICE_DATA,

    // 0xfcd2 - bthome.io service UUID
    0xd2,
    0xfc,

    // Service header - no encryption, bt home v2
    0x40,

    // Temperature.
    0x02, // Type Flag (Temperature)
    0x44, // 21,16 Degrees
    0x08, // 21,16 Degrees

    // Humidity.
    0x2E, // Type Flag (Humidity)
    0x51, // 81% rel

    // Illuminance.
    0x05, // Type Flag (Illuminance)
    0x02, // 10,26 lx (LUX)
    0x04, // 10,26 lx (LUX)
    0x00, // 10,26 lx (LUX)

    // Battery voltage.
    0x0C, // Type Flag (Battery Voltage)
    0xA8, // 2,984 V
    0x0B, // 2,984 V

    // Battery percentage
    0x01,
    0x5C, // 92%

    // Packet-Id
    0x00,
    1
};

void populate_service_data() {
  adc_sensor_data_t sensor_data;
  read_sensors(&sensor_data);

  uint32_t packet_id = HAL_RTCEx_BKUPRead(&hrtc, 1);
  if (packet_id >= 255) {
    packet_id = 0;
  }

  packet_id +=1;
  HAL_RTCEx_BKUPWrite(&hrtc, 1, packet_id);

  uint16_t temperature = (uint16_t) ((float) sensor_data.MCUTemperature * 100.0f);
  uint16_t battery_voltage = (uint16_t) sensor_data.VRefInt;
  uint32_t lux = (uint32_t) sensor_data.Brightness * 100;

  service_data[6] = temperature & 0xff;
  service_data[7] = temperature >> 8;

  service_data[11] = lux & 0xff;
  service_data[12] = lux >> 8;
  service_data[13] = lux >> 16;

  service_data[15] = battery_voltage & 0xff;
  service_data[16] = battery_voltage >> 8;

  service_data[18] = sensor_data.BatteryPercent;
  service_data[20] = packet_id;
}