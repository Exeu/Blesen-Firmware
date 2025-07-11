/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void) {
    /* USER CODE BEGIN RTC_Init 0 */

    /* USER CODE END RTC_Init 0 */

    /* USER CODE BEGIN RTC_Init 1 */

    /* USER CODE END RTC_Init 1 */

    /** Initialize RTC Only
    */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
    hrtc.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the WakeUp
    */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN RTC_Init 2 */

    /* USER CODE END RTC_Init 2 */
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (rtcHandle->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */

        /** Initializes the peripherals clock
        */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* RTC clock enable */
        __HAL_RCC_RTC_ENABLE();
        __HAL_RCC_RTCAPB_CLK_ENABLE();

        /* RTC interrupt Init */
        HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
        /* USER CODE BEGIN RTC_MspInit 1 */

        /* USER CODE END RTC_MspInit 1 */
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle) {
    if (rtcHandle->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspDeInit 0 */

        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
        __HAL_RCC_RTCAPB_CLK_DISABLE();

        /* RTC interrupt Deinit */
        HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
        /* USER CODE BEGIN RTC_MspDeInit 1 */

        /* USER CODE END RTC_MspDeInit 1 */
    }
}

void RTC_Alarm_IRQHandler(void) {
    /* USER CODE BEGIN RTC_Alarm_IRQn 0 */

    /* USER CODE END RTC_Alarm_IRQn 0 */
    HAL_RTC_AlarmIRQHandler(&hrtc);
    /* USER CODE BEGIN RTC_Alarm_IRQn 1 */

    /* USER CODE END RTC_Alarm_IRQn 1 */
}


/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
