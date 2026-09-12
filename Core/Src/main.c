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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

volatile uint8_t button_pressed_event = 0;
volatile uint8_t button_released_event = 0;
volatile uint8_t mode_button_event = 0;

volatile uint8_t tap_answer_count = 0;

volatile uint32_t button_press_time = 0;
volatile uint32_t button_release_time = 0;

int beat_count = -1;
int current_bpm = 0;

typedef enum
{
	STATE_WELCOME,     //시작 화면
	STATE_TAP_TEMPO,	//버튼 입력
	STATE_METRONOME,	//메트로놈 재생
	STATE_RHYTHM_PATTERN, //리듬 패턴 출력
	STATE_RHYTHM_INPUT, // 리듬 패턴 입력
	STATE_RESULT	//리듬 훈련 결과 표시
} SystemState;

SystemState current_state = STATE_WELCOME;

#define RHYTHM_SIZE 8

uint8_t rhythm_pattern[RHYTHM_SIZE];

uint8_t pattern_index = 0;

uint32_t rhythm_interval = 0;
uint32_t last_rhythm_time = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Beep(uint32_t ms);
void Display_Metronome_Status(uint32_t bpm, int beat);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = HAL_GetTick();

    if (GPIO_Pin == GPIO_PIN_10)
    {
    	uint32_t current_time = HAL_GetTick();

		// 마지막 인터럽트로부터 30ms 이내라면 bouncing으로 판단
		if (current_time - last_interrupt_time < 50)
		{
			return;
		}

		// 정상적인 edge로 인정
		last_interrupt_time = current_time;


		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10) == GPIO_PIN_RESET)
		{
			// 버튼 누름
			button_press_time = current_time;
			button_pressed_event = 1;
		}
		else
		{
			// 버튼 뗌
			button_release_time = current_time;
			button_released_event = 1;
		}
    }

    else if (GPIO_Pin == GPIO_PIN_13)
    {
        mode_button_event = 1;
    }
}


void Display_OLED() {
	  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	  SSD1306_Init();
	  SSD1306_GotoXY (0,0);
	  SSD1306_Puts ("-------------------", &Font_7x10, 1);
	  SSD1306_GotoXY (39, 18);
	  SSD1306_Puts ("WELCOME", &Font_7x10, 1);
	  SSD1306_GotoXY (57, 30);
	  SSD1306_Puts ("TO", &Font_7x10, 1);
	  SSD1306_GotoXY (43, 42);
	  SSD1306_Puts ("STempo", &Font_7x10, 1);
	  SSD1306_GotoXY (0, 53);
	  SSD1306_Puts ("------------------", &Font_7x10, 1);
	  SSD1306_UpdateScreen();
	  HAL_Delay (1000);
	  SSD1306_ScrollRight(0,7);
	  HAL_Delay(3000);
	  SSD1306_ScrollLeft(0,7);
	  HAL_Delay(3000);
	  SSD1306_Stopscroll();
	  SSD1306_Clear();


	  SSD1306_GotoXY (25,20);
	  SSD1306_Puts ("Are you", &Font_11x18, 1);
	  SSD1306_GotoXY (31,40);
	  SSD1306_Puts ("ready?", &Font_11x18, 1);
	  SSD1306_UpdateScreen();
	  HAL_Delay (100);
	  SSD1306_Clear();

	  SSD1306_GotoXY (7,27);
	  SSD1306_Puts ("SHOW YOUR TEMPO!", &Font_7x10, 1);
	  SSD1306_UpdateScreen();
	  HAL_Delay (200);
	  SSD1306_Clear();
}


void Generate_Rhythm_Pattern(void)
{
	tap_answer_count = 0;
    // 첫 번째 박은 항상 소리가 나도록 함
    rhythm_pattern[0] = 1;
    tap_answer_count++;

    // 나머지 7칸 랜덤 생성
    for (int i = 1; i < RHYTHM_SIZE; i++)
    {
        rhythm_pattern[i] = rand() % 2;
        if (rhythm_pattern[i]==1) {
        	tap_answer_count++;
        }
    }
}
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
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  Display_OLED(); //전원이 켜진 후 시작 화면 출력

  current_state = STATE_TAP_TEMPO;  //메트로놈 모드 진입, 템포 입력




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  static uint32_t last_tap_time = 0;
  static uint32_t diff = 0;
  static int count = 0;

  static uint32_t avg_interval = 0;
  static uint32_t last_beep_time = 0;
  static uint32_t last_tap_index = 0;
  static uint32_t answer_interval = 0;
  static uint32_t user_interval = 0;

  static uint8_t button_is_pressed = 0;
  static uint8_t long_press_triggered = 0;
  static uint8_t rhythm_pattern_started = 0;

  uint32_t rhythm_state_enter_time = 0;
  uint8_t rhythm_waiting = 1;

  static uint32_t first_input_time = 0;
  uint32_t user_tap_time[8];
  int user_tap_count = 0;

  uint8_t answer_count = 0;
  static bool result_processed = false;

  uint32_t total_abs_error = 0;
  int32_t total_signed_error = 0;
  uint8_t evaluated_taps = 0;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  switch (current_state) {

	  case STATE_WELCOME:
		  Display_OLED();
		  current_state = STATE_TAP_TEMPO;
		  break;

	  case STATE_TAP_TEMPO:
		  // 버튼이 눌렸을 때
		  if (button_pressed_event)
		  {

			  button_pressed_event = 0;

			  button_is_pressed = 1;
			  long_press_triggered = 0;
		  }


		  // Long Press 인지 확인

		  if (button_is_pressed &&
			  long_press_triggered == 0 &&
			  (HAL_GetTick() - button_press_time >= 1000))
		  {
			  // 메트로놈 상태 초기화


			  count = 0;
			  diff = 0;

			  avg_interval = 0;
			  last_tap_time = 0;

			  beat_count = -1;
			  current_bpm = 0;

			  long_press_triggered = 1;

			  // 초기화 되었다는 것을 소리&화면으로 알림.
			  Beep(300);

			  Display_Metronome_Status(0, -1);
		  }


		  // 버튼에서 손을 뗐을 때

		  if (button_released_event)
		  {
			  button_released_event = 0;

			  button_is_pressed = 0;

			  // Long Press가 아닐 경우
			  if (long_press_triggered == 0)
			  {
				  // 첫 번째 Tap
				  if (count == 0)
				  {

					  last_tap_time = button_press_time;

					  count = 1;

					  Beep(50);
				  }

				  // 2번째 ~ 4번째 Tap
				  else if (count > 0 && count < 4)
				  {
					  diff += (button_press_time - last_tap_time);

					  last_tap_time = button_press_time;

					  count++;

					  Beep(50);
				  }


				  // 4번의 Tap이 감지된 후 BPM 계산
				  if (count == 4)
				  {
					  avg_interval = diff / 3;

					  //0으로 나누는 상황 방지
					  if (avg_interval > 0)
					  {
						  current_bpm = 60000 / avg_interval;

						  beat_count = -1;

						  last_beep_time = HAL_GetTick();

						  current_state = STATE_METRONOME;

						  Display_Metronome_Status(
							  current_bpm,
							  beat_count
						  );
					  }

					  // 초기화
					  count = 0;
					  diff = 0;
					  last_tap_time = 0;
				  }
			  }

			  // Long Press 초기화
			  long_press_triggered = 0;
		  }

		  break;

	  case STATE_METRONOME:

	      // 버튼 눌림 이벤트 처리
	      if (button_pressed_event)
	      {
	          button_pressed_event = 0;

	          button_is_pressed = 1;
	          long_press_triggered = 0;
	      }

	      // 버튼 뗌 이벤트 처리
	      if (button_released_event)
	      {
	          button_released_event = 0;

	          button_is_pressed = 0;
	          long_press_triggered = 0;
	      }


	      // 메트로놈 재생
	      if (avg_interval > 0)
	      {
	          if (HAL_GetTick() - last_beep_time >= avg_interval)
	          {
	              last_beep_time = HAL_GetTick();

	              beat_count = (beat_count + 1) % 4;

	              Display_Metronome_Status(
	                  current_bpm,
	                  beat_count
	              );

	              Beep(32);
	          }
	      }


	      // 1초 이상 Long Press → Tap Tempo로 돌아가기
	      if (button_is_pressed &&
	          long_press_triggered == 0 &&
	          (HAL_GetTick() - button_press_time >= 1000))
	      {
	          count = 0;
	          diff = 0;

	          avg_interval = 0;
	          last_tap_time = 0;

	          beat_count = -1;
	          current_bpm = 0;

	          long_press_triggered = 1;

	          current_state = STATE_TAP_TEMPO;

	          Beep(300);
	          Display_Metronome_Status(0, -1);
	      }

	      if (mode_button_event)
	      {
	          mode_button_event = 0;
	          srand(HAL_GetTick());

	          Generate_Rhythm_Pattern();

	          rhythm_interval = avg_interval / 2;
	          last_rhythm_time = HAL_GetTick();

	          rhythm_state_enter_time = HAL_GetTick();
	          rhythm_waiting = 1;
	          pattern_index = 0;


	          current_state = STATE_RHYTHM_PATTERN;
	      }

	      break;


	  case STATE_RHYTHM_PATTERN:

		  mode_button_event = 0;

		  // 이 상태에 처음 들어왔을 때 한 번만 화면 출력
		  if (rhythm_pattern_started == 0)
		  {
			  SSD1306_Clear();

			  SSD1306_GotoXY(36, 11);
			  SSD1306_Puts("Remember", &Font_7x10, 1);

			  SSD1306_GotoXY(22, 43);
			  SSD1306_Puts("this pattern", &Font_7x10, 1);

			  SSD1306_UpdateScreen();

			  rhythm_pattern_started = 1;

			  rhythm_state_enter_time = HAL_GetTick();
			  rhythm_waiting = 1;
		  }


	      // 상태 진입 후 1.5초 동안은 패턴 출력하지 않음
	      if (rhythm_waiting)
	      {
	          if (HAL_GetTick() - rhythm_state_enter_time >= 1500)
	          {
	              rhythm_waiting = 0;

	              // 지금부터 리듬 타이밍 시작
	              last_rhythm_time = HAL_GetTick();
	          }

	          break;
	      }


	      // 1.5초가 지난 뒤부터 패턴 출력
	      if (HAL_GetTick() - last_rhythm_time >= rhythm_interval)
	      {
	          last_rhythm_time += rhythm_interval;

	          if (rhythm_pattern[pattern_index] == 1)
	          {
	              Beep(30);
	          }

	          pattern_index++;

	          if (pattern_index >= RHYTHM_SIZE)
	          {
	              pattern_index = 0;

	              button_pressed_event = 0;
	              button_released_event = 0;

	              user_tap_count = 0;

	              current_state = STATE_RHYTHM_INPUT;
	          }
	      }


	      break;

	  case STATE_RHYTHM_INPUT:

		  mode_button_event = 0;

	      if (button_pressed_event)
	      {
	          button_pressed_event = 0;

	          Beep(30);

	          // 첫 번째 입력
	          if (user_tap_count == 0)
	          {
	              first_input_time = button_press_time;

	              user_tap_time[0] = 0;

	              user_tap_count = 1;
	          }

	          // 두 번째 이후 입력
	          else if (user_tap_count < tap_answer_count)
	          {
	              user_tap_time[user_tap_count] =
	                  button_press_time - first_input_time;

	              user_tap_count++;
	          }
	      }

	      // 필요한 횟수만큼 입력했다면 결과 상태로
	      if (user_tap_count >= tap_answer_count)
	      {
	    	  user_tap_count = 0;
	    	  button_pressed_event = 0;
	    	  button_released_event = 0;

	    	  mode_button_event = 0;

	          current_state = STATE_RESULT;
	      }

	      break;

	  case STATE_RESULT:

	      if (result_processed == false)
	      {
	          answer_count = 0;
	          user_tap_count = 0;

	          uint32_t total_abs_error = 0;
	          int32_t total_signed_error = 0;
	          uint8_t evaluated_taps = 0;

	          for (int i = 0; i < RHYTHM_SIZE; i++)
	          {
	              if (rhythm_pattern[i] == 1)
	              {
	                  answer_interval = rhythm_interval * i;

	                  user_interval =
	                      user_tap_time[user_tap_count];

	                  user_tap_count++;

	                  //음수: 빠름 , 양수: 느림
	                  int32_t timing_error =
	                      (int32_t)user_interval -
	                      (int32_t)answer_interval;

	                  uint32_t abs_error;

	                  if (timing_error < 0)
	                      abs_error = (uint32_t)(-timing_error);
	                  else
	                      abs_error = (uint32_t)timing_error;



	                  if (abs_error <= 100)
	                  {
	                      answer_count++;
	                  }


	                  // 첫번째 Tap은 기준점이라 항상 error = 0이므로 분석 X
	                  if (i != 0)
	                  {
	                      //Accuracy가 음수가 되지 않도록 함.
	                      uint32_t error_for_score = abs_error;

	                      if (error_for_score > rhythm_interval)
	                      {
	                          error_for_score = rhythm_interval;
	                      }

	                      total_abs_error += error_for_score;
	                      total_signed_error += timing_error;

	                      evaluated_taps++;
	                  }
	              }
	          }


	          //Accuracy 및 Time bias 계산

	          if (evaluated_taps > 0 && rhythm_interval > 0)
	          {

	              uint32_t denominator =
	                  (uint32_t)evaluated_taps * rhythm_interval;

	              int32_t accuracy_x10 =
	                  1000 -
	                  (int32_t)(
	                      ((uint64_t)total_abs_error * 1000)
	                      / denominator
	                  );

	              if (accuracy_x10 < 0)
	                  accuracy_x10 = 0;


	              int32_t timing_bias =
	                  total_signed_error / evaluated_taps;


	              // UART 전송

	              char uart_buf[64];

	              sprintf(
	                  uart_buf,
	                  "SESSION,%ld,%ld\r\n",
	                  accuracy_x10,
	                  timing_bias
	              );

	              HAL_UART_Transmit(
	                  &huart2,
	                  (uint8_t *)uart_buf,
	                  strlen(uart_buf),
	                  HAL_MAX_DELAY
	              );
	          }


	          //OLED 결과 표시

	          SSD1306_Clear();

	          if (tap_answer_count - answer_count == 0)
	          {
	              SSD1306_GotoXY(36, 27);
	              SSD1306_Puts(
	                  "Perfect!",
	                  &Font_7x10,
	                  1
	              );
	          }
	          else if (tap_answer_count - answer_count == 1)
	          {
	              SSD1306_GotoXY(50, 27);
	              SSD1306_Puts(
	                  "Good",
	                  &Font_7x10,
	                  1
	              );
	          }
	          else
	          {
	              SSD1306_GotoXY(32, 27);
	              SSD1306_Puts(
	                  "Try Again",
	                  &Font_7x10,
	                  1
	              );
	          }

	          SSD1306_UpdateScreen();

	          result_processed = true;
	      }


	      //Mode 버튼 -> 다음 문제
	      if (mode_button_event)
	      {
	          mode_button_event = 0;

	          result_processed = false;
	          answer_count = 0;
	          user_tap_count = 0;

	          Generate_Rhythm_Pattern();

	          rhythm_interval = avg_interval / 2;

	          rhythm_state_enter_time = HAL_GetTick();

	          rhythm_waiting = 1;
	          rhythm_pattern_started = 0;
	          pattern_index = 0;

	          button_pressed_event = 0;
	          button_released_event = 0;

	          current_state = STATE_RHYTHM_PATTERN;
	      }

	      break;
	}


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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Beep(uint32_t ms)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 500);

    HAL_Delay(ms);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
}

void Display_Metronome_Status(uint32_t bpm, int beat)
{
    // 박자 상태 문자열
	char oled_beat_buf[20];
	char oled_bpm_buf[20];

    if (beat == 0)      strcpy(oled_beat_buf, "O  -  -  -");
    else if (beat == 1) strcpy(oled_beat_buf, "-  O  -  -");
    else if (beat == 2) strcpy(oled_beat_buf, "-  -  O  -");
    else if (beat == 3) strcpy(oled_beat_buf, "-  -  -  O");
    else                strcpy(oled_beat_buf, "-  -  -  -"); // 정지 상태 등

    // BPM 문자열
    if (bpm == 0) sprintf(oled_bpm_buf, "BPM: --");
    else          sprintf(oled_bpm_buf, "BPM: %lu", bpm);

    // OLED 출력
    // SSD1306_Clear(); // 화면 전체 지우기

    SSD1306_GotoXY(7, 5);
    SSD1306_Puts("SHOW YOUR TEMPO!", &Font_7x10, 1);

    SSD1306_GotoXY(10, 22);
    SSD1306_Puts(oled_bpm_buf, &Font_11x18, 1);


    SSD1306_GotoXY(15, 45);
    SSD1306_Puts(oled_beat_buf, &Font_11x18, 1);

    SSD1306_UpdateScreen();
}
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
