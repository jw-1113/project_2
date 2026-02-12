#ifndef INC_SEVEN_SEG_H_
#define INC_SEVEN_SEG_H_

#include "stm32f4xx_hal.h"

/* 7-Segment 제어 함수 */
void SevenSeg_Init(void);
void SevenSeg_Display(uint8_t num);


#endif /* INC_SEVEN_SEG_H_ */
