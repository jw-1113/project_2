#include "stepper.h"

/* ============================================================
 * 28BYJ-48 GPIO 핀 매핑 (ULN2003 IN1~IN4)
 * ------------------------------------------------------------
 * 28BYJ-48은 4개의 코일(상)을 순서대로 구동해야 회전함.
 * 각 INx는 ULN2003 드라이버 입력에 연결되어 있음.
 * ============================================================ */
#define STEPPER_IN1_PORT   GPIOC
#define STEPPER_IN1_PIN    GPIO_PIN_4

#define STEPPER_IN2_PORT   GPIOB
#define STEPPER_IN2_PIN    GPIO_PIN_15

#define STEPPER_IN3_PORT   GPIOB
#define STEPPER_IN3_PIN    GPIO_PIN_14

#define STEPPER_IN4_PORT   GPIOB
#define STEPPER_IN4_PIN    GPIO_PIN_13


/* ============================================================
 * Half-Step Drive Sequence (8-Phase)
 * ------------------------------------------------------------
 * Half-step은 Full-step보다 부드럽고 토크/해상도 균형이 좋음.
 * 28BYJ-48(감속기 포함)은 보통 Half-step 기준 4096 step = 1회전.
 * (64 steps/rev * 64 감속 = 4096)
 * ============================================================ */
static const uint8_t HALF_STEP_SEQ[8][4] =
{
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};


/* ============================================================
 * Speed Table (Step Frequency: Hz)
 * ------------------------------------------------------------
 * speedLevel(1~5)에 따라 "스텝 발생 빈도(Hz)"가 달라짐.
 * 타이머 인터럽트 주기를 이 값에 맞춰 설정해서 속도 제어.
 * ============================================================ */
static const uint16_t speedTable[] = { 900, 950, 1500, 1800, 2100 };
#define SPEED_TABLE_SIZE   (sizeof(speedTable)/sizeof(speedTable[0]))


/* ============================================================
 * 내부 유틸
 * ============================================================ */
static inline uint32_t Stepper_DegreesToSteps(uint32_t degrees)
{
	// steps = degrees * 4096 / 360
	// 32-bit overflow 방지를 위해 64-bit로 곱한 뒤 다시 32-bit로 캐스팅
    return (uint32_t)((uint64_t)degrees * 4096u / 360u);
}

static inline void Stepper_OutputStep(uint8_t stepIndex)
{
	// HALF_STEP_SEQ[stepIndex][0~3] 값을 각각 IN1~IN4에 출력
    HAL_GPIO_WritePin(STEPPER_IN1_PORT, STEPPER_IN1_PIN,
                      HALF_STEP_SEQ[stepIndex][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN2_PORT, STEPPER_IN2_PIN,
                      HALF_STEP_SEQ[stepIndex][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN3_PORT, STEPPER_IN3_PIN,
                      HALF_STEP_SEQ[stepIndex][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN4_PORT, STEPPER_IN4_PIN,
                      HALF_STEP_SEQ[stepIndex][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


/* ============================================================
 * Public API
 * ============================================================ */
void Stepper_Init(Stepper_t *motor, TIM_HandleTypeDef *htim)
{
    motor->state          = STEPPER_IDLE;		// 초기 상태: 정지(IDLE)
    motor->htim           = htim;				// 스텝 생성에 사용할 타이머 핸들
    motor->busy           = 0;					// 동작 중 여부 플래그
    motor->direction      = DIR_CW;			// 기본 방향/속도
    motor->speedLevel     = 1;
    motor->currentStep    = 0;					// 현재 시퀀스 위치(0~7)
    motor->remainingSteps = 0;					// 남은 스텝 수(0이면 정지)
}

void Stepper_SetSpeed(Stepper_t *motor)
{
    // speedLevel: 1~5로 클램프
    if(motor->speedLevel < 1) motor->speedLevel = 1;
    if(motor->speedLevel > (uint8_t)SPEED_TABLE_SIZE) motor->speedLevel = (uint8_t)SPEED_TABLE_SIZE;

    uint16_t hz = speedTable[motor->speedLevel - 1];

     // (가정) 타이머 업데이트 주기 = 1MHz 기반 (PSC 설정은 CubeMX/별도에서 맞춰둔다고 가정)
    // ARR = 1,000,000 / hz
    //    uint32_t arr = 1000000u / (uint32_t)hz;
    motor->htim->Instance->PSC = 100 - 1;
	motor->htim->Instance->ARR = (1000000u / hz) - 1;

    __HAL_TIM_SET_COUNTER(motor->htim, 0);
}

void Stepper_Start(Stepper_t *motor, uint32_t degrees, uint8_t direction, uint8_t speed)
{
    if(motor->busy) return;		// 이미 돌고 있으면 재시작 방지

    motor->direction      = direction;			// 회전 방향 저장
    motor->speedLevel     = speed;				// 속도 레벨 저장
    motor->remainingSteps = Stepper_DegreesToSteps(degrees);	// 목표 회전량(도→스텝)
    motor->busy           = 1;					// 동작 중 표시
    motor->state          = STEPPER_RUN;		// 상태 RUN으로 전환

    Stepper_SetSpeed(motor);					// 타이머 주기 설정
    HAL_TIM_Base_Start_IT(motor->htim);		// 타이머 인터럽트 시작(스텝 생성 시작)
}

void Stepper_Stop(Stepper_t *motor)
{
	// 남은 스텝을 0으로 만들어서
	// ISR/Task가 자연스럽게 STOP 상태로 넘어가게 함 (안전한 종료)
    motor->remainingSteps = 0;
}

uint8_t Stepper_IsBusy(Stepper_t *motor)
{
    return motor->busy;		// 1이면 동작 중, 0이면 정지
}

void Stepper_Task(Stepper_t *motor)
{
    switch(motor->state)
    {
        case STEPPER_IDLE:		// 아무것도 안함
            break;

        case STEPPER_RUN:
            if(motor->remainingSteps == 0)		// 남은 스텝이 0이 되면 STOP 상태로 전환
                motor->state = STEPPER_STOP;
            break;

        case STEPPER_STOP:
        	// 타이머 인터럽트 정지 → 스텝 생성 중단
            HAL_TIM_Base_Stop_IT(motor->htim);
            motor->busy  = 0;					// 이제 정지 상태
            motor->state = STEPPER_IDLE;		// IDLE 복귀
            break;

        default:
        	// 예외 방어: 상태가 이상하면 강제 초기화
            motor->state = STEPPER_IDLE;
            motor->busy  = 0;
            break;
    }
}

void Stepper_ISR(Stepper_t *motor)
{
    if(motor->remainingSteps == 0) return;		// 정지 요청/완료면 아무것도 안함

    // 다음 시퀀스 인덱스 계산
    if(motor->direction == DIR_CW)
        motor->currentStep = (motor->currentStep + 1) & 0x07; // 0~7 순환 (%8)
    else
        motor->currentStep = (motor->currentStep == 0) ? 7 : (motor->currentStep - 1);

    // 코일 출력(실제 회전 발생)
    Stepper_OutputStep(motor->currentStep);

    // 남은 스텝 감소
    motor->remainingSteps--;
}
