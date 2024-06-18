#ifndef BLESEN_FW_STM32_WPAN_APP_ADVERTISER_H_
#define BLESEN_FW_STM32_WPAN_APP_ADVERTISER_H_

#include <stdint.h>

void Adv_Start(uint8_t *sd, uint8_t size_t);
void Adv_Init();

#endif //BLESEN_FW_STM32_WPAN_APP_ADVERTISER_H_
