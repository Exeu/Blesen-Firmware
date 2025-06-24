// NOT USED ATM!

#include "advertiser-extended.h"
#include "ble_types.h"
#include "ble_gap_aci.h"
#include "ble_core.h"
#include "app_conf.h"
#include "blesen-service.h"

typedef struct
{
  uint8_t enable;
  Adv_Set_t adv_set;
  uint8_t* data;
  uint16_t data_len;
  uint8_t sid;
  uint16_t property;
  uint16_t interval_min;
  uint16_t interval_max;
  int8_t tx_power;
  uint8_t adv_channels;
  uint8_t address_type;
  uint8_t peer_address_type;
  uint8_t* p_peer_address;
  char username[30];
} Adv_Set_Param_t;

static uint8_t ADV_EXT_Build_data(Adv_Set_Param_t *adv_param);

Adv_Set_Param_t adv_set_param[8];

void Adv_ext_Start() {
  tBleStatus status = BLE_STATUS_INVALID_PARAMS;
  uint8_t handle = 0;

  int i = 0;
  adv_set_param[i].enable = 1;
  adv_set_param[i].data = NULL;
  adv_set_param[i].data_len = 1650;
  strcpy(adv_set_param[i].username, "ext_noscan_noconn");
  adv_set_param[i].sid = i;
  adv_set_param[i].interval_min = 300;
  adv_set_param[i].interval_max = 400;
  adv_set_param[i].tx_power = CFG_BLE_MAX_TX_POWER;
  adv_set_param[i].adv_channels = ADV_CH_37 | ADV_CH_38 | ADV_CH_39;
  adv_set_param[i].property = 0x00;
  adv_set_param[i].address_type = GAP_PUBLIC_ADDR;
  adv_set_param[i].peer_address_type = GAP_PUBLIC_ADDR;
  adv_set_param[i].p_peer_address = NULL;
  adv_set_param[i].adv_set.Advertising_Handle = 0;
  adv_set_param[i].adv_set.Duration = 0;
  adv_set_param[i].adv_set.Max_Extended_Advertising_Events = 0;

  ADV_EXT_Build_data(&adv_set_param[i]);

  status = aci_gap_adv_set_configuration(
    0x01,
    handle,
    0x0001,
    300,
    400,
    ADV_CH_37 | ADV_CH_38 | ADV_CH_39,
    GAP_PUBLIC_ADDR,
    GAP_PUBLIC_ADDR,
    NULL,
    NO_WHITE_LIST_USE,
    0x00,
    0x01,
    0x01,
    1,
    0x00);

  if (status != BLE_STATUS_SUCCESS) {
    printf("error");
  }
  adv_set_param[0].data = &service_data[0];

  status = aci_gap_adv_set_adv_data(
      adv_set_param[0].adv_set.Advertising_Handle,
      HCI_SET_ADV_DATA_OPERATION_COMPLETE,
    0,
    251,
    &adv_set_param[0].data[0]);
  if (status != BLE_STATUS_SUCCESS) {
    printf("error");
  }
  status = aci_gap_adv_set_enable(1, 1, &adv_set_param[0].adv_set);
  if (status != BLE_STATUS_SUCCESS) {
    printf("error");
  }
}

static uint8_t ADV_EXT_Build_data(Adv_Set_Param_t *adv_param)
{
  uint16_t data_cpt, cpt;
  int16_t manuf_cpt;
  uint8_t i, status = 0;

  data_cpt = 0;
  memset(&adv_param->data[0], 0, adv_param->data_len);

  adv_param->data[data_cpt++] = 0x02;
  adv_param->data[data_cpt++] = AD_TYPE_FLAGS;
  adv_param->data[data_cpt++] = FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE | FLAG_BIT_BR_EDR_NOT_SUPPORTED;

  adv_param->data[data_cpt++] = strlen(adv_param->username) + 1;
  adv_param->data[data_cpt++] = AD_TYPE_COMPLETE_LOCAL_NAME;
  memcpy(&adv_param->data[data_cpt], &adv_param->username[0], strlen(adv_param->username));
  data_cpt += strlen(adv_param->username);

  manuf_cpt = (adv_param->data_len - data_cpt);
  if(manuf_cpt <= 2)
  {
    status = 1;
  }
  else
  {
    while (manuf_cpt > 0)
    {
      if (manuf_cpt >= 0xFE)
      {
        cpt = 0xFE - 2;
      }
      else
      {
        cpt = manuf_cpt - 2;
      }
      adv_param->data[data_cpt++] = cpt + 1;
      adv_param->data[data_cpt++] = AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
      adv_param->data[data_cpt++] = 0x30; /* STMicroelectronis company ID */
      cpt--;
      adv_param->data[data_cpt++] = 0x00;
      cpt--;
      for (i = 0 ; i < cpt; i++)
      {
        adv_param->data[data_cpt + i] = i;
      }
      data_cpt += cpt;

      manuf_cpt = (adv_param->data_len - data_cpt);
    }
  }

  return status;
}