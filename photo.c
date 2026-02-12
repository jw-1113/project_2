/*
 * photo.c
 *
 *  Created on: 2026. 2. 6.
 *      Author: user17
 */

#include "photo.h"

volatile uint8_t floor = 1;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)      // PB0 → 1층
    {
        floor = 1;
    }
    else if (GPIO_Pin == GPIO_PIN_1) // PB1 → 2층
    {
        floor = 2;
    }
    else if (GPIO_Pin == GPIO_PIN_2) // PB2 → 3층
    {
        floor = 3;
    }
}
