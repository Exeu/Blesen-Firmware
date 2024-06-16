#include "blesen-service.h"
#include "app_conf.h"
#include "ble_gap_aci.h"
#include "ble_hci_le.h"
#include "otp.h"
#include "ble_core.h"
#include "adc-sensors.h"

static void populate_service_data(adc_sensor_data_t *sensor_data);

static const char a_LocalName[] = {
    AD_TYPE_COMPLETE_LOCAL_NAME,
    'B',
    'L',
    'S',
    'N'
};

uint8_t flags[] =
    {
        2,
        AD_TYPE_FLAGS,
        (FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE | FLAG_BIT_BR_EDR_NOT_SUPPORTED)
    };

uint8_t serviceData[19] = {
    sizeof(serviceData) - 1,
    AD_TYPE_SERVICE_DATA,

    // 0xfcd2 - bthome.io service UUID.
    0xd2,
    0xfc,

    // Service header - no encryption, bt home v2.
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

    // Battery percentage.
    0x01,
    0x5C // 92%
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
      sizeof(a_LocalName),
      (uint8_t *) &a_LocalName,
      0,
      NULL,
      0,
      0);

  /* Remove the TX power level advertisement (this is done to decrease the packet size). */
  aci_gap_delete_ad_type(AD_TYPE_TX_POWER_LEVEL);
  /* Update the service data. */
  aci_gap_update_adv_data(sizeof(serviceData), serviceData);
  /* Update the adverstising flags. */
  aci_gap_update_adv_data(sizeof(flags), flags);
}

static void populate_service_data(adc_sensor_data_t *sensor_data) {
  uint16_t temperature = (uint16_t) ((float) sensor_data->MCUTemperature * 100.0f);
  uint16_t battery_voltage = (uint16_t) sensor_data->VRefInt;
  uint32_t lux = (uint32_t) sensor_data->Brightness * 100;

  serviceData[6] = temperature & 0xff;
  serviceData[7] = temperature >> 8;

  serviceData[11] = lux & 0xff;
  serviceData[12] = lux >> 8;
  serviceData[13] = lux >> 16;

  serviceData[15] = battery_voltage & 0xff;
  serviceData[16] = battery_voltage >> 8;

  serviceData[18] = sensor_data->BatteryPercent;
}