#include "advertiser.h"
#include "ble_gap_aci.h"
#include "stm32_seq.h"
#include "ble_hci_le.h"
#include "adc.h"
#include "i2c.h"
#include "ipcc.h"
#include "stm32_lpm.h"
#include "rtc.h"
#include "ble_core.h"
#include "blesen-service.h"
#include "lptim.h"

#define INITIAL_ADV_TIMEOUT (0.5*1000*1000/CFG_TS_TICK_VAL) /** 0.5s */

static const char local_name[] = {
    AD_TYPE_COMPLETE_LOCAL_NAME,
    'B',
    'L',
    'S',
    'N'
};

typedef struct {
  uint8_t Advertising_mgr_timer_Id;
} Adv_Context_t;

static Adv_Context_t AdvContext;
static void Adv_Mgr(void);

void Adv_Init() {
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &(AdvContext.Advertising_mgr_timer_Id), hw_ts_SingleShot, Adv_Mgr);
}

void Adv_Start(uint8_t *sd, uint8_t size_t) {
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
  aci_gap_update_adv_data(size_t, sd);

  HW_TS_Start(AdvContext.Advertising_mgr_timer_Id, INITIAL_ADV_TIMEOUT);
}

static void Adv_Mgr(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  aci_gap_set_non_discoverable(); // Setting off BLE activities. CPU2 can now enter power saving modes

  UTIL_SEQ_PauseTask(UTIL_SEQ_DEFAULT);
  hci_reset();

  HAL_ADC_MspDeInit(&hadc1);
  HAL_I2C_DeInit(&hi2c1);
  HAL_IPCC_MspDeInit(&hipcc);

  // Disabling all GPIO related clocks
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOH_CLK_DISABLE();
  __HAL_RCC_GPIOB_CLK_DISABLE();
  __HAL_RCC_GPIOA_CLK_DISABLE();
  __HAL_RCC_GPIOE_CLK_DISABLE();

  // Setting the previously used pins into analog mode.
  // Analog mode is deactivating any internal circuit which reduces power consumption
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Allowing the LPM to enter Off-Mode aka StandBy (second-lowest power consumption)
  UTIL_LPM_SetStopMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);
  UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);

  // Activate low power wake-up timer
  uint8_t wakeup_counter = 40; // 40 is around 11 minutes
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  HAL_RTC_MspDeInit(&hrtc);

  if (HAL_LPTIM_TimeOut_Start_IT(&hlptim1, 65535, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /**
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeup_counter, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) {
    Error_Handler();
  } */
}