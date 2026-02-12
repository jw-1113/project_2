/*
 * button.c
 *
 *  Created on: 2026. 1. 23.
 *      Author: user17
 */

#include "button.h"

BUTTON_CONTROL button[9]={
		{GPIOA, GPIO_PIN_5, 0},	// 내부 1번
		{GPIOA, GPIO_PIN_6, 0},	// 내부 2번
		{GPIOA, GPIO_PIN_7,0},	// 내부 3번
		{GPIOB, GPIO_PIN_3, 0},	// 외부 1층버튼
		{GPIOB, GPIO_PIN_4, 0},	// 외부 2층버튼 하강
		{GPIOB, GPIO_PIN_5, 0},	// 외부 2층버튼 상승
		{GPIOB, GPIO_PIN_6, 0},	// 외부 3층버튼
		{GPIOA, GPIO_PIN_10, 0},	// 열림
		{GPIOA, GPIO_PIN_9, 0},	// 닫힘

		};

bool ButtonGetPressd(uint8_t num)
{
    static uint32_t prevTime[9] = {0};
    if (num >= 9) return false;

    if (HAL_GPIO_ReadPin(button[num].port, button[num].number) == button[num].onState)
    {
        uint32_t currTime = HAL_GetTick();
        if (currTime - prevTime[num] > 200)
        {
            prevTime[num] = currTime;
            return true;
        }
    }
    return false;
}
