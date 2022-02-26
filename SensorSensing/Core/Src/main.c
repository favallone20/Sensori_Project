/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define MSG_SIZE 1
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  int CAMPIONI = 210;
  uint32_t value[2];
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, 2);
  uint32_t v[CAMPIONI];
  uint32_t i[CAMPIONI];
  double phase[50];
  char str[6];
  char phase_msg[8];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int counter = 0;
  long double SAMPLE_TIME = 0.000095;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_ADC_Stop_DMA(&hadc1);
	  if(counter < CAMPIONI){
	  	  i[counter] = value[0];
	  	  v[counter] = value[1];
	  }
	  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, 2);
	  counter++;

	  if(counter-1 == CAMPIONI){
		  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)){
			  HAL_UART_Transmit(&huart2, "Tensione:\n", sizeof("Tensione:\n"), HAL_MAX_DELAY);
			  HAL_UART_Transmit(&huart1, "Tensione\n", sizeof("Tensione\n"), HAL_MAX_DELAY);
			  for(int j = 0; j < CAMPIONI; j++){
				  sprintf(str, " %lu ", v[j]);
				  HAL_UART_Transmit(&huart2, str, sizeof(str), HAL_MAX_DELAY);
				  sprintf(str, "%lu", v[j]);
				  HAL_UART_Transmit(&huart1, str, sizeof(str), HAL_MAX_DELAY);
			  }
			  HAL_UART_Transmit(&huart1, "\n", sizeof("\n"), HAL_MAX_DELAY);


			  HAL_UART_Transmit(&huart2, "\n\nCorrente:\n", sizeof("\n\nCorrente:\n"), HAL_MAX_DELAY);
			  HAL_UART_Transmit(&huart1, "Corrente\n", sizeof("Corrente\n"), HAL_MAX_DELAY);
			  for(int j = 0; j < CAMPIONI; j++){
				  sprintf(str, " %lu ", i[j]);
				  HAL_UART_Transmit(&huart2, str, sizeof(str), HAL_MAX_DELAY);
				  sprintf(str, "%lu", i[j]);
				  HAL_UART_Transmit(&huart1, str, sizeof(str), HAL_MAX_DELAY);
			  }
			  HAL_UART_Transmit(&huart1, "\n", sizeof("\n"), HAL_MAX_DELAY);


			  uint32_t diff = v[CAMPIONI - 1] - v[1];
			  int k = 2;

			  while((v[CAMPIONI - k] - v[1]) < diff){
				  diff = v[CAMPIONI - k] - v[1];
				  k++;
			  }

			  int min_v = v[0];
			  int min_v_index = 0;
			  for(int j = 1; j <= CAMPIONI - k + 1; j++){
				  if(v[j] < min_v){
					  min_v = v[j];
					  min_v_index = j;
				  }
			  }

			  uint32_t start_i = i[1];
			  diff = i[CAMPIONI - 1] - i[1];
			  k = 2;

			  while((i[CAMPIONI - k] - i[1]) < diff){
				  diff = i[CAMPIONI - k] - i[1];
				  k++;
			  }

			  int min_i = i[0];
			  int min_i_index = 0;
			  for(int j = 1; j < CAMPIONI - k + 1; j++){
				  if(i[j] < min_i){
					  min_i = i[j];
					  min_i_index = j;
				  }
			  }

			  double td = SAMPLE_TIME*(min_i_index - min_v_index);
			  double phase_shift = (360*td)/0.02;

			  HAL_UART_Transmit(&huart1, "Fase\n", sizeof("Fase\n"), HAL_MAX_DELAY);
			  gcvt(phase_shift, 6, phase_msg);
			  HAL_UART_Transmit(&huart1, phase_msg, sizeof(phase_msg), HAL_MAX_DELAY);
			  HAL_UART_Transmit(&huart1, "\n", sizeof("\n"), HAL_MAX_DELAY);
		  }
		  counter = 0;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
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

#ifdef  USE_FULL_ASSERT
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

