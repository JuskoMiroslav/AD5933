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
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

#include "uart_hal.h"
#include "ad5933.h"
#include <cstdio>
//#include <stdio.h>

const char* commands[3] = {"START_FREQ","STEP_FREQ","STEP_COUNT"};
const char* initTest[5] = {"Reset","StatFreq","StepFreq","StepCount","OutputRange"};


void SystemClock_Config(void);

void recvUart(UartDMA& uart1,bool& newData, char* str);

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  UartDMA uart1(USART2,&huart2);
  HAL_Delay(1000);
  int StartFreq = 10;
  int StepFreq = 10;
  int StepCount = 10;
  bool  StartSweep = false;
  AD5933 ad5933(&hi2c1);
  for(int i = 0;i< 6;i++){
	  HAL_StatusTypeDef test;
	  switch(i){
	  case 0:
		  test = ad5933.reset();
		  break;
	  case 1:
		  test = ad5933.setExternalClockSource(true);
		  break;
	  case 2:
		  test = ad5933.setStartFrequency(StartFreq);
		  break;
	  case 3:
		  test = ad5933.setIncrementFrequency(StepFreq);
		  break;
	  case 4:
		  test = ad5933.setNumberIncrements(StepCount);
		  break;
	  case 5:
		  test = ad5933.setOutputRange(OUTPUT_RANGE_2);
		  break;
	  }
	  if(test != HAL_OK)
	  {
		  uart1.write((uint8_t*)initTest[i],strlen(initTest[i])*sizeof(uint8_t));
		  while(!uart1.is_tx_done());
		  while(1);
	  }
  }

  uart1.write((uint8_t*)"<Initialization succes>",23);
  char str[100] = {0};
  uart1.start_read();
  bool newData = false;
  while (1)
  {
	  int* real;
	  int* imag;
//	  if(uart1.avalible())
//		  HAL_Delay(100);

	  recvUart(uart1, newData,str);
	  if(newData)
	  {
		  for(int i = 0;i < 3;i++){
			  int len = strlen(commands[i]);
			  if(!strncmp(str,(char*)commands[i],len)){
				  char* pStart = &str[len+1];
				  int val = strtol(pStart,NULL,10);
				  switch(i){
				  case 0:
					  StartFreq =val;
					  ad5933.setStartFrequency(StartFreq);
					  break;
				  case 1:
					  StepFreq = val;
					  ad5933.setIncrementFrequency(StepFreq);
					  break;
				  case 2:
					  StepCount = val;
					  ad5933.setNumberIncrements(StepCount);
					  real = (int*)malloc(StepCount*sizeof(int));
					  imag = (int*)malloc(StepCount*sizeof(int));
					  break;
				  }
			  }
			  if(!strcmp(str,(char*)"START_SWEEP") && !StartSweep)
				  StartSweep = true;	  }
		  uart1.write((uint8_t*)"<OK>",4);
		  while(!uart1.is_tx_done());
//		  memset(str,0,100*sizeof(char));
		  newData = false;
		  memset(str,0,sizeof(str));
	  }
	  if(StartSweep == true){
		  if(ad5933.frequencySweep(real, imag, StepCount)== HAL_OK){
			  float freq = StartFreq/1000.0;
			  uart1.write((uint8_t*)"<SweepDone>",11);
			  while(!uart1.is_tx_done());
			  for(int i = 0;i < StepCount;i++){
				  sprintf(str,"f:%f, R:%d, I:%d\n",freq,real[i],imag[i]);
				  uart1.write((uint8_t*)str,strlen(str)*sizeof(char));
				  while(!uart1.is_tx_done());
				  freq +=StepFreq/1000.0;

				  memset(str,0,sizeof(str));
//				  HAL_Delay(100);
			  }
			  StartSweep = false;
			  free(real);
			  free(imag);
		  }
		  else
			  HAL_Delay(1);
	  }

  }
}

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
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void recvUart(UartDMA& uart1,bool& newData, char* str){

//	bool newData = false;
	static bool recvInProgress = false;
	static uint8_t ndx = 0;
	uint8_t rc[1];
	 while(uart1.avalible() > 0 && newData == false)
		  {
		   uart1.read(rc, 1);

			   if(recvInProgress){
				   if(rc[0] != '>'){
					   str[ndx] = (char)rc[0];
					   ndx++;
//					   uart1.write(&rc[0], 1);
//					   while(!uart1.is_tx_done());
				   }
				   else{
					   str[ndx] = '\0';
					   recvInProgress = false;
					   ndx = 0;
					   newData = true;
				   }
			   }
			   else if(rc[0] == '<')
				   recvInProgress = true;
		  }
}
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
