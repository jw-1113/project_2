#include "led_bar.h"

typedef struct {
    GPIO_TypeDef* port;   // LED가 연결된 GPIO 포트
    uint16_t pin;         // LED가 연결된 핀 번호
} LED_t;

static LED_t led_bar[8] = {
    {GPIOC, GPIO_PIN_9},   // 0 : TOP (맨 위 LED)
    {GPIOB, GPIO_PIN_8},   // 1
    {GPIOB, GPIO_PIN_9},   // 2
    {GPIOC, GPIO_PIN_8},   // 3
    {GPIOC, GPIO_PIN_6},   // 4
    {GPIOC, GPIO_PIN_5},   // 5
    {GPIOA, GPIO_PIN_12},  // 6
    {GPIOA, GPIO_PIN_11}   // 7 : BOTTOM (맨 아래 LED)
};

// 상승용 변수
static int led_index_up = 7;			// 상승 시작 위치 (맨 아래 LED)
static uint32_t led_timer_up = 0;		// 마지막 LED 변경 시간 저장용
static uint8_t first_run_up = 1;		// 첫 실행 여부 (즉시 실행용 플래그)

// 하강용 변수
static int led_index_down = 0;			// 하강 시작 위치 (맨 위 LED)
static uint32_t led_timer_down = 0;	// 마지막 LED 변경 시간
static uint8_t first_run_down = 1;		// 첫 실행 여부

void LED_BAR_Init(void)
{
    LED_All_Off();       // 모든 LED 끄기

    led_index_up = 7;    // 상승 시작점 리셋
    led_index_down = 0;  // 하강 시작점 리셋

    first_run_up = 1;
    first_run_down = 1;  // 첫 실행 플래그 초기화

    led_timer_up = 0;
    led_timer_down = 0;  // 타이머 초기화
}

void LED_All_Off(void)
{
    for(int i = 0; i < 8; i++)
        {
            HAL_GPIO_WritePin(led_bar[i].port,
                              led_bar[i].pin,
                              GPIO_PIN_RESET);
        }
}

void LED_Sequential_Up(void)
{
    // 첫 실행이거나 150ms가 지났으면
    if(first_run_up || HAL_GetTick() - led_timer_up >= 150)
    {
        first_run_up = 0;     // 첫 실행 플래그 제거
        LED_All_Off();        // 이전 LED 끄기
        // 현재 위치 LED 켜기
        HAL_GPIO_WritePin(led_bar[led_index_up].port,
                          led_bar[led_index_up].pin,
                          GPIO_PIN_SET);

        led_index_up--;       // 위로 이동 (index 감소)
        // 맨 위 지나면 다시 아래로 순환
        if(led_index_up < 0)
            led_index_up = 7;

        led_timer_up = HAL_GetTick();  // 시간 저장
    }
}

void LED_Sequential_Down(void)
{
    if(first_run_down || HAL_GetTick() - led_timer_down >= 150)
    {
        first_run_down = 0;
        LED_All_Off();
        HAL_GPIO_WritePin(led_bar[led_index_down].port,
                          led_bar[led_index_down].pin,
                          GPIO_PIN_SET);

        led_index_down++;   // 아래 방향 (index 증가)
        // 맨 아래 지나면 다시 위로
        if(led_index_down > 7)
            led_index_down = 0;

        led_timer_down = HAL_GetTick();
    }
}
