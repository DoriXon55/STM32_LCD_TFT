/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include <stdbool.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi.h"
#include "lcd.h"
#include "USART_ringbuffer.h"
#include "frame.h"
#include "hagl.h"

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
extern uint8_t USART_RxBuf[];
extern uint8_t USART_TxBuf[];
extern volatile int USART_TX_Empty;
extern volatile int USART_RX_Busy;
extern volatile int USART_RX_Empty;
extern volatile int USART_TX_Busy;
volatile uint32_t tick;
char bx[128];
bool escape_detected = false;
uint8_t bx_index = 0;
bool in_frame = false;
char received_char;
Receive_Frame ramka;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay(uint32_t delayMs);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay(uint32_t delayMs){
	uint32_t startTime = tick;
	while(tick < (startTime+delayMs)); //niestety blokuje działanie programu ale na szczęście nie przerwań
}
void reset_frame_state() {
    in_frame = false;
    escape_detected = false;
    bx_index = 0;
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
  SysTick_Config( 80000000 / 1000 ); //ustawienie systicka na 1 ms
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2,&USART_RxBuf[0],1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  if (USART_kbhit()) {                // Sprawdzamy, czy jest dostępny nowy znak
	          received_char = USART_getchar();   // Pobieramy znak z bufora odbiorczego

	          if (received_char == '~') {    // Rozpoczęcie nowej ramki
	              if (!in_frame) {
	                  USART_fsend("Znaleziono poczatek ramki...\r\n");
	                  in_frame = true;
	                  bx_index = 0;           // Resetujemy indeks bufora
	                  escape_detected = false; // Resetujemy flagę escape
	              } else {
	                  reset_frame_state();
	              }
	          } else if (received_char == '`') {    // Koniec ramki
	              if (in_frame) {
	                  USART_fsend("Koniec odbioru, nastepuje sprawdzanie ramki...\r\n");
	                  // Przetwarzanie odebranej ramki (np. wywołanie funkcji lub ustawienie flagi)
	                  if (decodeFrame(bx,&ramka, bx_index)) {
	                      USART_fsend("SUKCES!\r\n");
	                      USART_fsend("%d", bx_index);
	                      handleCommand(&ramka);
	                  } else {
	                      USART_fsend("BŁĄD: Dekodowanie ramki nie powiodło się\r\n");
	                  }

	                  // Resetujemy stany po przetworzeniu ramki
	                  reset_frame_state();
	              } else {
	                  // Jeśli ramka się kończy, ale nie została rozpoczęta
	                  USART_fsend("BŁĄD: Zakonczenie ramki bez rozpoczecia\r\n");
	                  reset_frame_state();
	              }
	          } else if (in_frame) {
	              // Jesteśmy w ramce i przetwarzamy znaki
	              if (escape_detected) {
	                  // Jeśli wykryto escape char, sprawdzamy następny znak
	                  if (received_char == '^') {
	                      bx[bx_index++] = '~'; // '~' było zakodowane jako '}^'
	                  } else if (received_char == ']') {
	                      bx[bx_index++] = '}'; // '}' było zakodowane jako '}]'
	                  } else if (received_char == '&') {
	                      bx[bx_index++] = '`'; // '`' było zakodowane jako '}&'
	                  } else {
	                      // Nieprawidłowy znak po '}', resetujemy
	                      USART_fsend("BŁĄD: Nieprawidłowy escape sequence\r\n");
	                      reset_frame_state();
	                  }
	                  escape_detected = false; // Resetujemy flagę escape
	              } else if (received_char == '}') {
	                  escape_detected = true; // Wykryto znak escape, czekamy na następny
	              } else {
	                  // Normalny znak w ramce, zapisujemy do bx
	                  if (bx_index < sizeof(bx)) {
	                      bx[bx_index++] = received_char;
	                  } else {
	                      // kontrola przepełnienia
	                      reset_frame_state();
	                  }
	              }
	          } else {
	              // Ignorujemy znaki poza ramką, ale resetujemy dla bezpieczeństwa
	              //USART_fsend("BŁĄD: Znak poza ramką\r\n");
	              //TODO bujnowski moze sie przyjebac ze jest reset co pętle
	              reset_frame_state();
	          }
	      }
	 	       //HAL_Delay(250);
	 	       //HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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
