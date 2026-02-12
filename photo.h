/*
 * photo.h
 *
 *  Created on: 2026. 2. 6.
 *      Author: user17
 */

#ifndef INC_PHOTO_H_
#define INC_PHOTO_H_

#include "stm32f4xx_hal.h"

extern volatile uint8_t floor;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* INC_PHOTO_H_ */
