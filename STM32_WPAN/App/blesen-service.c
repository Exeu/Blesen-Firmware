#include "blesen-service.h"
#include "app_conf.h"
#include "ble_gap_aci.h"
#include "ble_hci_le.h"
#include "ble_core.h"
#include "adc-sensors.h"
#include "rtc.h"

static void populate_service_data(adc_sensor_data_t *sensor_data);

static const char local_name[] = {
    AD_TYPE_COMPLETE_LOCAL_NAME,
    'B',
    'L',
    'S',
    'N'
};

uint8_t flags[] = {
    2,
    AD_TYPE_FLAGS,
    (FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE | FLAG_BIT_BR_EDR_NOT_SUPPORTED)
};

uint8_t service_data[21] = {
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

void Adv_Start(void) {
  adc_sensor_data_t sa;
  read_sensors(&sa);
  populate_service_data(&sa);

  hci_le_set_scan_response_data(0, NULL);

  aci_gap_set_discoverable(
      ADV_NONCONN_IND,
      CFG_FAST_CONN_ADV_INTERVAL_MIN,
      CFG_FAST_CONN_ADV_INTERVAL_MAX,
      GAP_PUBLIC_ADDR,
      NO_WHITE_LIST_USE, /* use white list */
      sizeof(local_name),
      (uint8_t *) &local_name,
      0,
      NULL,
      0,
      0);

  /* Remove the TX power level advertisement (this is done to decrease the packet size). */
  aci_gap_delete_ad_type(AD_TYPE_TX_POWER_LEVEL);
  /* Update the service data. */
  aci_gap_update_adv_data(sizeof(service_data), service_data);
  /* Update the adverstising flags. */
  aci_gap_update_adv_data(sizeof(flags), flags);
}

static void populate_service_data(adc_sensor_data_t *sensor_data) {
  uint32_t packet_id = HAL_RTCEx_BKUPRead(&hrtc, 1);
  if (packet_id >= 255) {
    packet_id = 0;
  }

  packet_id +=1;
  HAL_RTCEx_BKUPWrite(&hrtc, 1, packet_id);

  uint16_t temperature = (uint16_t) ((float) sensor_data->MCUTemperature * 100.0f);
  uint16_t battery_voltage = (uint16_t) sensor_data->VRefInt;
  uint32_t lux = (uint32_t) sensor_data->Brightness * 100;

  service_data[6] = temperature & 0xff;
  service_data[7] = temperature >> 8;

  service_data[11] = lux & 0xff;
  service_data[12] = lux >> 8;
  service_data[13] = lux >> 16;

  service_data[15] = battery_voltage & 0xff;
  service_data[16] = battery_voltage >> 8;

  service_data[18] = sensor_data->BatteryPercent;
  service_data[20] = packet_id;
}