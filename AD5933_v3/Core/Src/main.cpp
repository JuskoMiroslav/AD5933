#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

#include "uart_hal.h"
#include "ad5933.h"
#include <cstdio>
//#include <stdio.h>

const char* commands[4] = {"START_FREQ","STEP_FREQ","STEP_COUNT","OUTPUT_RANGE"};
const char* initTest[5] = {"Reset","StatFreq","StepFreq","StepCount","OutputRange"};
const char * errormsg = "Error in initialization, in function ";

void SystemClock_Config(void);

void recvUart(UartDMA& uart1,bool& newData, char* str);
bool freqSweep(UartDMA &uart1,AD5933 &ad5933,int16_t real[],int16_t imag[],int n);
void parseData(char* str);
double gain[10];


int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  char str[100] = {0};
  parseData((char*)"StartSweep StartFrequency=100 StepFrequency=10 StepCount=2");
  bool newData = false;
  UartDMA uart1(USART2,&huart2);
  uart1.start_read();
//  HAL_Delay(1000);
  while(newData != true && strcmp(str,"Connected") != 0){
  	  recvUart(uart1, newData, str);
  	  HAL_Delay(100);
  }
  newData = false;
  int StartFreq = 10;
  int StepFreq = 10;
  int StepCount = 10;
  bool  StartSweep = false;
  bool  GetData = false;
  AD5933 ad5933(&hi2c1);
  uint8_t res = ad5933.init(10000, 1000, 10, OUTPUT_RANGE_2);
  if(res != 0)
	  {

	  	  uart1.write((uint8_t*)"<",1);
	  	  uart1.write((uint8_t*)errormsg,strlen(errormsg)*sizeof(uint8_t));

		  uart1.write((uint8_t*)initTest[res],strlen(initTest[res])*sizeof(uint8_t));

		  uart1.write((uint8_t*)">",1);

		  while(1);
	  }
  uart1.write((uint8_t*)"<Initialization succes>",23);
  ad5933.calibrate(gain, 8200, 10);
  while (1)
  {
	  int16_t* real;
	  int16_t* imag;
//	  if(uart1.avalible())
//		  HAL_Delay(100);

	  recvUart(uart1, newData,str);
	  if(newData)
	  {
		  HAL_StatusTypeDef res = HAL_OK;

		  for(int i = 0;i < 3;i++){
			  int len = strlen(commands[i]);
			  if(!strncmp(str,(char*)commands[i],len)){
				  char* pStart = &str[len+1];
				  int val = strtol(pStart,NULL,10);

				  switch(i){
				  case 0:
					  StartFreq =val;
					  res = ad5933.setStartFrequency(StartFreq);
					  break;
				  case 1:
					  StepFreq = val;
					  res = ad5933.setIncrementFrequency(StepFreq);
					  break;
				  case 2:
					  StepCount = val;
					  res = ad5933.setNumberIncrements(StepCount);
					  real = (int16_t*)malloc(StepCount*sizeof(int16_t));
					  imag = (int16_t*)malloc(StepCount*sizeof(int16_t));
					  break;
				  case 3:
					  res = ad5933.setNumberIncrements(val);
				  }
			  }
		  }
		  if(!strcmp(str,(char*)"START_SWEEP") && !StartSweep) {
			  ad5933.setPowerMode(POWER_STANDBY);
			  ad5933.reset();
			  StartSweep = true;
		  }
		  if(!strcmp(str,(char*)"GET_DATA") && !StartSweep) {
			  GetData = true;
		  		  }
		  if(res != HAL_OK)
			  uart1.write((uint8_t*)"<NOK>",5);

		  uart1.write((uint8_t*)"<OK> ",5);

//		  memset(str,0,100*sizeof(char));
		  newData = false;
		  memset(str,0,sizeof(str));
	  }
	  if(StartSweep == true){

//		  if(ad5933.frequencySweep(real, imag, StepCount)== HAL_OK){
		  if(freqSweep(uart1, ad5933, real, imag, StepCount)){

			  uart1.write((uint8_t*)"<SweepDone>",11);

			  StartSweep = false;
		  }
		  else
			  HAL_Delay(1);
	  }
	  if(GetData == true)
	  {
		  float freq = StartFreq/1000.0;
		  uart1.write((uint8_t*)"<",1);
		  for(int i = 0;i < StepCount;i++){
			  sprintf(str,"f:%f, R:%d, I:%d\n",freq,real[i],imag[i]);
			  uart1.write((uint8_t*)str,strlen(str)*sizeof(char));

			  freq +=StepFreq/1000.0;

			  memset(str,0,sizeof(str));
//				  HAL_Delay(100);
		  }
		  GetData = false;
		  free(real);
		  free(imag);
		  uart1.write((uint8_t*)">",1);
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
//
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

bool freqSweep(UartDMA &uart1,AD5933 &ad5933,int16_t real[],int16_t imag[],int n){
	if(ad5933.setPowerMode(POWER_STANDBY) != HAL_OK ||
	   ad5933.setControlRegister(CTRL_INIT_START_F) != HAL_OK ||
	   ad5933.setControlRegister(CTRL_START_F_SWEEP) != HAL_OK)
		return false;
	int  i = 0;
	while((ad5933.readRegister(STATUS_REG) & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE){
//		while((ad5933.readRegister(STATUS_REG) &STATUS_DATA_VALID) != STATUS_DATA_VALID)
//				{
//
//				if(HAL_GetTick()%1000 == 0)
//					uart1.write((uint8_t*)"Running   ", 11);
//				}

		if(ad5933.getComplexData(&real[i], &imag[i]) != HAL_OK)
			return false;
		if(i >= n)
			return false;
		i++;
		ad5933.setControlRegister(CTRL_INCREMENT_F);
		while((ad5933.readRegister(STATUS_REG) &STATUS_DATA_VALID) != STATUS_DATA_VALID)
			if(HAL_GetTick()%1000 == 0)
			uart1.write((uint8_t*)"<running> ", 11);
		}
	return ad5933.setPowerMode(POWER_STANDBY) == HAL_OK;
}

void parseData(char* str){
	uint8_t mode = 0;
	char* rest = (char*)malloc(strlen(str));
	int StartFreq =0;
	int IncrFreq =0;
	int IncrCount = 0;
	int RepeatCount =0;
	strcpy(rest,str);
	char* token = strtok_r(rest," ",&rest);
	const char* commands[3] ={"StartSweep","RepeatSweep","Calibrate"};
	const char* modifiers[] = {"StartFrequency","StepFrequency","StepCount","RepeatCount"};
	if(token != NULL){
		for(int i = 0;i < 3;i++){
			if(strcmp(token,commands[i]) == 0){
				mode = i;
				break;
			}
		}
		while(token != NULL){
		for(int i = 0; i < 4;i++){
			  int len = strlen(modifiers[i]);
			  if(!strncmp(token,(char*)modifiers[i],len)){
				  char* pStart = &token[len+1];
				  int val = strtol(pStart,NULL,10);
				  switch(i){
				  	  case 0:
				  		  StartFreq = val;
				  		  break;

				  	  case 1:
				  		  IncrFreq = val;
				  		  break;
				  	  case 2:
				  		  IncrCount = val;
				  		  break;
				  	  case 3:
				  		  RepeatCount = val;
				  		  break;
					}
			  }
			}
			token = strtok_r(NULL," ", &rest);
		}

	}
	free(rest);
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
