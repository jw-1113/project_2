/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "photo.h"
#include "button.h"
#include "led_bar.h"
#include "seven_seg.h"
#include "stepper.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

typedef enum { DIR_NONE=0, DIR_UP, DIR_DOWN } Dir_t;		// 엘리베이터 이동 방향 상태
static volatile uint8_t target_floor = 1;					// 목표 층 (1~3층)
static volatile uint8_t moving = 0;						// 0 = 정지 상태, 1 = 이동 중
static volatile Dir_t dir = DIR_NONE;						// 현재 이동 방향
static uint8_t prev_floor = 0;								// 이전 층 값 (7세그 갱신용 비교 변수)

Stepper_t stepper;			// 스테퍼 모터 구조체
typedef enum { DOOR_IDLE=0, DOOR_OPENING, DOOR_WAITING, DOOR_CLOSING } DoorState_t;
// 문 상태 머신
// IDLE      : 대기
// OPENING   : 열림 동작
// WAITING   : 열린 상태 유지
// CLOSING   : 닫힘 동작

static volatile DoorState_t doorState = DOOR_IDLE;		// 현재 문 상태
static uint32_t doorTimer = 0;								// 문 열림 유지 시간 측정용 타이머
#define SERVO_OPEN_CCR   40									// 서보 열림 위치 PWM 값
#define SERVO_CLOSE_CCR  250								// 서보 닫힘 위치 PWM 값
#define DOOR_HOLD_MS     5000								// 문 열림 유지 시간 (5초)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static void StartMoveTo(uint8_t tf)
{
    if (tf < 1 || tf > 3) return;
    // 잘못된 층 번호 방지

    target_floor = tf;

    // 이미 해당 층이면 아무 동작 안함
    if (floor == target_floor)
    {
        moving = 0;
        dir = DIR_NONE;
        LED_All_Off();
        Stepper_Stop(&stepper);
        return;
    }

    // 이동 방향 결정
    dir = (floor < target_floor) ? DIR_UP : DIR_DOWN;

    moving = 1;  // 이동 시작

    LED_BAR_Init();  // LED 애니메이션 초기화

    // 스테퍼 모터 시작
    // 충분히 큰 step 값으로 회전
    // 도착하면 아래 루프에서 Stop으로 강제 정지
    if (dir == DIR_UP)
        Stepper_Start(&stepper, 200000, DIR_CW, 1);
    else
        Stepper_Start(&stepper, 200000, DIR_CCW, 1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM10)
  {
    Stepper_ISR(&stepper);		// 타이머 인터럽트마다 스테퍼 한 스텝 처리
  }
}

static void Door_Task(void)
{
    uint32_t now = HAL_GetTick();
    // 현재 ms 시간

    switch (doorState)
    {
    case DOOR_OPENING:
        TIM2->CCR1 = SERVO_OPEN_CCR;  // 서보 열기
        doorTimer = now;             // 시간 저장
        doorState = DOOR_WAITING;    // 열린 상태로 전환
        break;

    case DOOR_WAITING:
        // 5초 유지 후 자동 닫힘
        if (now - doorTimer >= DOOR_HOLD_MS)
        {
            doorState = DOOR_CLOSING;
        }
        break;

    case DOOR_CLOSING:
        TIM2->CCR1 = SERVO_CLOSE_CCR; // 닫기
        doorState = DOOR_IDLE;       // 대기 상태로 복귀
        break;

    default:
        break;
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */
  SevenSeg_Init();
  SevenSeg_Display(floor);   // 시작 층 표시

  Stepper_Init(&stepper, &htim10);				// 스테퍼 모터 초기화
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);	// 서보 PWM 시작
  TIM2->CCR1 = SERVO_CLOSE_CCR;   				// 시작 시 문 닫힘 상태로 세팅

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* ===== 내부 호출 버튼 ===== */
	      if (ButtonGetPressd(0)) StartMoveTo(1);
	      if (ButtonGetPressd(1)) StartMoveTo(2);
	      if (ButtonGetPressd(2)) StartMoveTo(3);

	      /* ===== 문(서보) 버튼 ===== */
	      if (ButtonGetPressd(7))    // 열림 버튼
	      {
	          doorState = DOOR_IDLE;          // ✅ 자동 타이머 끔
	          TIM2->CCR1 = SERVO_OPEN_CCR;    // ✅ 그냥 열기만
	      }

	      if (ButtonGetPressd(8))    // 닫힘 버튼
	      {
	          doorState = DOOR_IDLE;          // ✅ 자동 타이머 끔
	          TIM2->CCR1 = SERVO_CLOSE_CCR;   // ✅ 닫기
	      }

	      /* ===== 외부 호출 버튼 ===== */
	      if (ButtonGetPressd(3)) StartMoveTo(1);
	      if (ButtonGetPressd(4)) StartMoveTo(2);
	      if (ButtonGetPressd(5)) StartMoveTo(2);
	      if (ButtonGetPressd(6)) StartMoveTo(3);

	      /* ===== 이동 중 LED 애니메이션 ===== */
	      if (moving)
	      {
	          if (dir == DIR_UP)        LED_Sequential_Up();
	          else if (dir == DIR_DOWN) LED_Sequential_Down();

	          // 목표 층 도착 확인
	          if (floor == target_floor)
	          {
	              moving = 0;
	              dir = DIR_NONE;
	              LED_All_Off();
	              Stepper_Stop(&stepper);

				  // 도착하면 문 자동 오픈
				  if (doorState == DOOR_IDLE)
					  doorState = DOOR_OPENING;
	          }
	      }

	      /* ===== 현재 층 값만 갱신(논블럭킹) ===== */
	      if (floor != prev_floor)
	      {
	          prev_floor = floor;
	          SevenSeg_Display(floor);
	      }

	      Door_Task();				// 문 상태 머신
	      SevenSeg_Task();       	// 7세그 멀티플렉싱 출력
	      Stepper_Task(&stepper);  // 스테퍼 메인 로직

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
