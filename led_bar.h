#ifndef INC_LED_BAR_H_
#define INC_LED_BAR_H_

#include "stm32f4xx_hal.h"

#define LED_UP     -1    // 하강 → 상승 (아래 → 위)
#define LED_DOWN    1    // 상승 → 하강 (위 → 아래)

void LED_BAR_Init(void);
void LED_All_Off(void);
void LED_Sequential_Up(void);
void LED_Sequential_Down(void);

#endif /* INC_LED_BAR_H_ */
