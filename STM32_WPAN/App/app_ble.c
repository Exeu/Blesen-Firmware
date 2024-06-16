#include "app_ble.h"

#include "app_conf.h"
#include "hci_tl.h"
#include "stm32_seq.h"
#include "stm32_lpm.h"
#include "main.h"
#include "ble_gap_aci.h"
#include "ble_gatt_aci.h"
#include "ble_hal_aci.h"
#include "ble_hci_le.h"
#include "ble_conf.h"
#include "otp.h"
#include "ble_core.h"
#include "rtc.h"
#include "svc_ctl.h"
#include "shci.h"
#include "blesen-service.h"

static void Ble_Tl_Init(void);
static void BLE_StatusNot(HCI_TL_CmdStatus_t Status);
static void BLE_UserEvtRx(void *p_Payload);
const uint8_t *BleGetBdAddress(void);
static void Ble_Hci_Gap_Gatt_Init(void);
static void Adv_Mgr(void);

#define BD_ADDR_SIZE_LOCAL    6
#define INITIAL_ADV_TIMEOUT            (0.5*1000*1000/CFG_TS_TICK_VAL) /**< 1s */
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

#define ENABLE_ALTERNATE_LUX_FORMULA    1

static const char a_LocalName[] = {
    AD_TYPE_COMPLETE_LOCAL_NAME,
    'B',
    'L',
    'S',
    'N'
};

static const uint8_t a_MBdAddr[BD_ADDR_SIZE_LOCAL] =
    {
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x0000000000FF)),
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x00000000FF00) >> 8),
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x000000FF0000) >> 16),
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x0000FF000000) >> 24),
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x00FF00000000) >> 32),
        (uint8_t) ((CFG_ADV_BD_ADDRESS & 0xFF0000000000) >> 40)
    };

static uint8_t a_BdAddrUdn[BD_ADDR_SIZE_LOCAL];

/**
 *   Identity root key used to derive IRK and DHK(Legacy)
 */
static const uint8_t a_BLE_CfgIrValue[16] = CFG_BLE_IR;

/**
 * Encryption root key used to derive LTK(Legacy) and CSRK
 */
static const uint8_t a_BLE_CfgErValue[16] = CFG_BLE_ER;

#define APPBLE_GAP_DEVICE_NAME_LENGTH 7
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

typedef struct {
  uint8_t Advertising_mgr_timer_Id;
} BleApplicationContext_t;

static BleApplicationContext_t BleApplicationContext;

void APP_BLE_Init(void) {
  SHCI_CmdStatus_t status;
  /* USER CODE BEGIN APP_BLE_Init_1 */

  /* USER CODE END APP_BLE_Init_1 */
  SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet =
      {
          {{0, 0, 0}},                          /**< Header unused */
          {0,                                 /** pBleBufferAddress not used */
           0,                                 /** BleBufferSize not used */
           CFG_BLE_NUM_GATT_ATTRIBUTES,
           CFG_BLE_NUM_GATT_SERVICES,
           CFG_BLE_ATT_VALUE_ARRAY_SIZE,
           CFG_BLE_NUM_LINK,
           CFG_BLE_DATA_LENGTH_EXTENSION,
           CFG_BLE_PREPARE_WRITE_LIST_SIZE,
           CFG_BLE_MBLOCK_COUNT,
           CFG_BLE_MAX_ATT_MTU,
           CFG_BLE_PERIPHERAL_SCA,
           CFG_BLE_CENTRAL_SCA,
           CFG_BLE_LS_SOURCE,
           CFG_BLE_MAX_CONN_EVENT_LENGTH,
           CFG_BLE_HSE_STARTUP_TIME,
           CFG_BLE_VITERBI_MODE,
           CFG_BLE_OPTIONS,
           0,
           CFG_BLE_MAX_COC_INITIATOR_NBR,
           CFG_BLE_MIN_TX_POWER,
           CFG_BLE_MAX_TX_POWER,
           CFG_BLE_RX_MODEL_CONFIG,
           CFG_BLE_MAX_ADV_SET_NBR,
           CFG_BLE_MAX_ADV_DATA_LEN,
           CFG_BLE_TX_PATH_COMPENS,
           CFG_BLE_RX_PATH_COMPENS,
           CFG_BLE_CORE_VERSION,
           CFG_BLE_OPTIONS_EXT
          }
      };

  Ble_Tl_Init();

  /**
 * Register the hci transport layer to handle BLE User Asynchronous Events
 */
  UTIL_SEQ_RegTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, hci_user_evt_proc);

  UTIL_LPM_SetStopMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);

  status = SHCI_C2_BLE_Init(&ble_init_cmd_packet);
  if (status != SHCI_Success) {
    /* if you are here, maybe CPU2 doesn't contain STM32WB_Copro_Wireless_Binaries, see Release_Notes.html */
    Error_Handler();
  }

  Ble_Hci_Gap_Gatt_Init();

  SVCCTL_Init();

  hci_le_set_scan_response_data(0, NULL);

  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &(BleApplicationContext.Advertising_mgr_timer_Id), hw_ts_SingleShot, Adv_Mgr);
  Adv_Start();
  HW_TS_Start(BleApplicationContext.Advertising_mgr_timer_Id, INITIAL_ADV_TIMEOUT);
}

static void Adv_Mgr(void) {
  aci_gap_set_non_discoverable();

  UTIL_LPM_SetStopMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);

  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 1, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) {
    Error_Handler();
  }
}

static void Ble_Hci_Gap_Gatt_Init(void) {
  uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
  const uint8_t *p_bd_addr;
  uint16_t a_appearance[1] = {BLE_CFG_GAP_APPEARANCE};

  /**
   * Initialize HCI layer
   */
  /*HCI Reset to synchronise BLE Stack*/
  hci_reset();

  /**
   * Write the BD Address
   */
  p_bd_addr = BleGetBdAddress();
  aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, (uint8_t *) p_bd_addr);

  /**
   * Write Identity root key used to derive IRK and DHK(Legacy)
   */
  aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t *) a_BLE_CfgIrValue);

  /**
   * Write Encryption root key used to derive LTK and CSRK
   */
  aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t *) a_BLE_CfgErValue);

  /**
   * Set TX Power.
   */
  aci_hal_set_tx_power_level(1, CFG_TX_POWER);

  /**
   * Initialize GATT interface
   */
  aci_gatt_init();

  aci_gap_init(
      1,
      0,
      APPBLE_GAP_DEVICE_NAME_LENGTH,
      &gap_service_handle,
      &gap_dev_name_char_handle,
      &gap_appearance_char_handle);

  aci_gatt_update_char_value(
      gap_service_handle,
      gap_dev_name_char_handle,
      0,
      strlen(a_LocalName),
      (uint8_t *) a_LocalName);

  aci_gatt_update_char_value(
      gap_service_handle,
      gap_appearance_char_handle,
      0,
      2,
      (uint8_t *) &a_appearance);
}

const uint8_t *BleGetBdAddress(void) {
  uint8_t *p_otp_addr;
  const uint8_t *p_bd_addr;
  uint32_t udn;
  uint32_t company_id;
  uint32_t device_id;

  udn = LL_FLASH_GetUDN();

  if (udn != 0xFFFFFFFF) {
    company_id = LL_FLASH_GetSTCompanyID();
    device_id = LL_FLASH_GetDeviceID();

    /**
     * Public Address with the ST company ID
     * bit[47:24] : 24bits (OUI) equal to the company ID
     * bit[23:16] : Device ID.
     * bit[15:0] : The last 16bits from the UDN
     * Note: In order to use the Public Address in a final product, a dedicated
     * 24bits company ID (OUI) shall be bought.
     */
    a_BdAddrUdn[0] = (uint8_t) (udn & 0x000000FF);
    a_BdAddrUdn[1] = (uint8_t) ((udn & 0x0000FF00) >> 8);
    a_BdAddrUdn[2] = (uint8_t) device_id;
    a_BdAddrUdn[3] = (uint8_t) (company_id & 0x000000FF);
    a_BdAddrUdn[4] = (uint8_t) ((company_id & 0x0000FF00) >> 8);
    a_BdAddrUdn[5] = (uint8_t) ((company_id & 0x00FF0000) >> 16);

    p_bd_addr = (const uint8_t *) a_BdAddrUdn;
  } else {
    p_otp_addr = OTP_Read(0);
    if (p_otp_addr) {
      p_bd_addr = ((OTP_ID0_t *) p_otp_addr)->bd_address;
    } else {
      p_bd_addr = a_MBdAddr;
    }
  }

  return p_bd_addr;
}

static void Ble_Tl_Init(void) {
  HCI_TL_HciInitConf_t Hci_Tl_Init_Conf;

  Hci_Tl_Init_Conf.p_cmdbuffer = (uint8_t *) &BleCmdBuffer;
  Hci_Tl_Init_Conf.StatusNotCallBack = BLE_StatusNot;
  hci_init(BLE_UserEvtRx, (void *) &Hci_Tl_Init_Conf);

  return;
}

static void BLE_UserEvtRx(void *p_Payload) {}

static void BLE_StatusNot(HCI_TL_CmdStatus_t Status) {
  uint32_t task_id_list;
  switch (Status) {
    case HCI_TL_CmdBusy:
      /**
       * All tasks that may send an aci/hci commands shall be listed here
       * This is to prevent a new command is sent while one is already pending
       */
      task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
      UTIL_SEQ_PauseTask(task_id_list);
      break;

    case HCI_TL_CmdAvailable:
      /**
       * All tasks that may send an aci/hci commands shall be listed here
       * This is to prevent a new command is sent while one is already pending
       */
      task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
      UTIL_SEQ_ResumeTask(task_id_list);
      /* USER CODE BEGIN HCI_TL_CmdAvailable */

      /* USER CODE END HCI_TL_CmdAvailable */
      break;

    default:break;
  }

  return;
}

void hci_notify_asynch_evt(void *p_Data) {
  UTIL_SEQ_SetTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);

  return;
}

void hci_cmd_resp_release(uint32_t Flag) {
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);

  return;
}

void hci_cmd_resp_wait(uint32_t Timeout) {
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);

  return;
}

