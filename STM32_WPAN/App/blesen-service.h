//
// Created by Jan Eichhorn on 14.06.24.
//

#ifndef BLESEN_FW_STM32_WPAN_APP_BLESEN_SERVICE_H_
#define BLESEN_FW_STM32_WPAN_APP_BLESEN_SERVICE_H_

#include "app_common.h"

#define SERVICE_DATA_LENGTH 21

void populate_service_data();
extern uint8_t service_data[SERVICE_DATA_LENGTH];

#endif //BLESEN_FW_STM32_WPAN_APP_BLESEN_SERVICE_H_
