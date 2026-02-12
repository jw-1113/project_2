#include "seven_seg.h"

// a b c d e f g
static GPIO_TypeDef* seg_port[7] = {
    GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC
};

static volatile uint8_t seg_value = 1;

static uint16_t seg_pin[7] = {
        GPIO_PIN_11,  // a
        GPIO_PIN_10,  // b
        GPIO_PIN_12,  // c
        GPIO_PIN_3,   // d
        GPIO_PIN_2,   // e
        GPIO_PIN_1,   // f
        GPIO_PIN_0    // g
};

/* 숫자 패턴 (1~3만 사용, 공통 캐소드) */
static const uint8_t seg_num[4][7] = {
/*0*/ {0,0,0,0,0,0,0},
/*1*/ {0,1,1,0,0,0,0},
/*2*/ {1,1,0,1,1,0,1},
/*3*/ {1,1,1,1,0,0,1}
};

void SevenSeg_Init(void)
{
    for(int i=0;i<7;i++)
            HAL_GPIO_WritePin(seg_port[i], seg_pin[i], GPIO_PIN_RESET);
}

void SevenSeg_Display(uint8_t num)
{
    if (num < 1 || num > 3) return;
    // 1~3 범위 외 숫자 무시

	seg_value = num;   // 값만 저장 (출력은 Task에서)
}

void SevenSeg_Task(void)
{
    uint8_t num = seg_value;

    for (int i = 0; i < 7; i++)
    {
        HAL_GPIO_WritePin(
            seg_port[i],
            seg_pin[i],
            seg_num[num][i] ? GPIO_PIN_SET : GPIO_PIN_RESET
        );
    }
}
