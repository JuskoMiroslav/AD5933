#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

#include "gpio_hal.h"
#include "uart_hal.h"
#include "ad5933.h"
#include <cstdio>
#include "math.h"
#include <stdio.h>
#include <vector>
#include <complex>
#include <array>
//const char* commands[4] = {"START_FREQ","STEP_FREQ","STEP_COUNT","OUTPUT_RANGE"};

const char *initTest[5] =
{ "Reset", "StatFreq", "StepFreq", "StepCount", "OutputRange" };
const char *errormsg = "Error in initialization, in function ";

void
SystemClock_Config(void);

void
recvUart(UartDMA &uart1, bool &newData, char *str);
//bool
//freqSweep(UartDMA &uart1, AD5933 &ad5933, std::vector<int16_t> &real,
//		std::vector<int16_t> &imag, settings set);

bool sweepAtFreq(UartDMA &uart1, AD5933 &ad5933, settings set, std::complex<double> ref,Dout led);
uint8_t
parseData(char *str, settings *set);
bool
setParameters(AD5933 &ad5933, settings set);

double absoluteAngleDiffrence(double angle1, double angle2);

int main(void)
{
	HAL_Init();

	SystemClock_Config();

	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	MX_I2C1_Init();
	char str[100] =
	{ 0 };
	settings set;
	std::vector<std::complex<double>> reference;
	std::vector<std::complex<double>> measured;

	set.SettlingCycles = 1;
	bool newData = false;
	UartDMA uart1(USART2, &huart2);
	uart1.start_read();
	Dout LED1(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
	Dout LED2(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	Dout LED3(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	Dout OscSw(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	Dout OutSw(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	LED1.write(GPIO_PIN_SET);

	OscSw.write(GPIO_PIN_SET);
	while (newData != true && strcmp(str, "Connected") != 0)
	{
		recvUart(uart1, newData, str);
//  	  HAL_Delay(100);
	}
	AD5933 ad5933(&hi2c1);
	uint8_t res = ad5933.init(10000, 1000, 10, OUTPUT_RANGE_1);
	ad5933.frequencySweep(reference, 50, 50);

	newData = false;
//  parseData((char*)"StartSweep StartFrequency=100 StepFrequency=10 StepCount=2",&set);

	bool StartSweep = false;
	bool GetData = false;
//  uint8_t repeatCount = 1;
	if (res != 0)
	{

		uart1.write((uint8_t*) "<", 1);
		uart1.write((uint8_t*) errormsg, strlen(errormsg) * sizeof(uint8_t));

		uart1.write((uint8_t*) initTest[res],
				strlen(initTest[res]) * sizeof(uint8_t));

		uart1.write((uint8_t*) ">", 1);

		while (1)
			;
	}
//	std::vector<float> gain;
//	std::vector<int16_t> real;
//	std::vector<int16_t> imag;
	uart1.write((uint8_t*) "<Initialization succes>", 23);
//	ad5933.calibrate(gain, 8200, 50, real, imag);
	while (1)
	{
//	  if(uart1.avalible())
//		  HAL_Delay(100);

		recvUart(uart1, newData, str);
		if (newData)
		{

			switch (parseData(str, &set))
			{
			case 1:
				ad5933.setPowerMode(POWER_STANDBY);
				ad5933.reset();
				StartSweep = true;
				uart1.write((uint8_t*) "<Sweep Started>", 15);
				break;
			case 2:
				OutSw.write(GPIO_PIN_SET);
				reference.clear();
				ad5933.setNumberIncrements(1);
				ad5933.frequencySweep(reference, 1, 50);

				OutSw.write(GPIO_PIN_RESET);
				uart1.write((uint8_t*) "<Continuous Started>", 20);
				sweepAtFreq(uart1, ad5933, set, reference.at(0),LED3);
				setParameters(ad5933, set);
				break;
			case 3:
				OutSw.write(GPIO_PIN_SET);
//				gain.clear();
//				ad5933.calibrate(gain, 820, set.IncrementCount, real, imag);
				reference.clear();
				ad5933.frequencySweep(reference, set.IncrementCount, 50);
				OutSw.write(GPIO_PIN_RESET);

				uart1.write((uint8_t*) "<Callibrated>", 13);

				break;
			case 4:
				GetData = true;
				break;
			case 5:
				if (setParameters(ad5933, set))
					uart1.write((uint8_t*) "<Parameters set>", 16);
				else
					uart1.write((uint8_t*) "<Parameters not set>", 20);

				break;
			case 7:
				uart1.write((uint8_t*) "<Already connected>", 19);
				break;
			default:
				;
			}
//			if (GetData != true)
//			{
//				if (!setParameters(ad5933, set))
//					uart1.write((uint8_t*) "<NOK>", 5);
//				else
//					uart1.write((uint8_t*) "<OK> ", 5);
//			}
			newData = false;
			memset(str, 0, sizeof(str));
		}
		if (StartSweep == true)
		{
//			real.clear();
//			imag.clear();
			measured.clear();

//		  if(ad5933.frequencySweep(real, imag, StepCount)== HAL_OK){

//			if (freqSweep(uart1, ad5933, real, imag, set))
			if (ad5933.frequencySweep(measured, set.IncrementCount,
					set.RepeatCount) == HAL_OK)
			{

				uart1.write((uint8_t*) "<SweepDone>", 11);

				StartSweep = false;
			}
			else
				HAL_Delay(1);
		}
		if (GetData == true)
		{
			float freq = set.StartFreq / 1000.0;
			uart1.write((uint8_t*) "<", 1);
//			for (int i = 0; i < set.IncrementCount; i++)
			unsigned int iteration = 0;
			for (const auto &num : measured)
			{
				memset(str, 0, sizeof(str));
				double magnitude = std::abs(reference.at(iteration));
				double Zabs = 1 / (std::abs(num) * (1 / 424.3) / magnitude);

				double phi = absoluteAngleDiffrence(std::arg(num),
						std::arg(reference.at(iteration)));
				sprintf(str, "f:%8.3f, |Z|:%10.3f, ph:%10.3f\n\r", freq, Zabs,
						phi);
				uart1.write((uint8_t*) str, strlen(str) * sizeof(char));
				iteration++;
//				if(iteration < measured.size())
//					uart1.write((uint8_t*) "\n\r", 2);
				freq += set.IncrementFreq / 1000.0;
				HAL_Delay(2);
			}
			uart1.write((uint8_t*) ">>>>>", 5);
			measured.clear();
			GetData = false;
		}

	}
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct =
	{ 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct =
	{ 0 };

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
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
}

void recvUart(UartDMA &uart1, bool &newData, char *str)
{

//	bool newData = false;
	static bool recvInProgress = false;
	static uint8_t ndx = 0;
	uint8_t rc[1];
	while (uart1.avalible() > 0 && newData == false)
	{
		uart1.read(rc, 1);

		if (recvInProgress)
		{
			if (rc[0] != '>')
			{
				str[ndx] = (char) rc[0];
				ndx++;
//					   uart1.write(&rc[0], 1);
//
			}
			else
			{
				str[ndx] = '\0';
				recvInProgress = false;
				ndx = 0;
				newData = true;
			}
		}
		else if (rc[0] == '<')
			recvInProgress = true;
	}
}

bool sweepAtFreq(UartDMA &uart1, AD5933 &ad5933, settings set, std::complex<double> ref, Dout led)
{
	if (ad5933.setPowerMode(POWER_STANDBY) != HAL_OK
			|| ad5933.setControlRegister(CTRL_INIT_START_F) != HAL_OK
			|| ad5933.setControlRegister(CTRL_START_F_SWEEP) != HAL_OK
			|| ad5933.setStartFrequency(set.StartFreq))
	{
		ad5933.setPowerMode(POWER_STANDBY);
		return false;
	}
	char rec[50] =
	{ 0 };
	bool newData = false;
	std::complex<double> meas;
	uint32_t oldTick = HAL_GetTick();
	char str[100] =
	{ 0 };
	while (!(newData && strcmp(rec, "<STOP>")))
	{


		recvUart(uart1, newData, rec);
		int16_t re = 0;
		int16_t im = 0;
		ad5933.setControlRegister(CTRL_REPEAT_FREQ);
		while ((ad5933.readRegister(STATUS_REG) & STATUS_DATA_VALID)
				!= STATUS_DATA_VALID)
			;

		if (ad5933.getComplexData(&re, &im) != HAL_OK)
			return false;
		std::complex<double> meas(re, im);
		double Zabs = 1 / (std::abs(meas) * (1 / 424.3) / std::abs(ref));
		double phi = absoluteAngleDiffrence(std::arg(meas),std::arg(ref));
//		memset(str, 0, sizeof(str));

		sprintf(str, "<f:%ld |Z|:%8.3f, ph:%8.3f>",HAL_GetTick()-oldTick, std::arg(meas), phi);
//		sprintf(str, "<f:%ld |Z|:%8.3f, ph:%8.3f>",HAL_GetTick()-oldTick, Zabs, phi);

		uart1.write((uint8_t*) str, strlen(str) * sizeof(char));
		while (HAL_GetTick() - oldTick < 5)
			;
		oldTick = HAL_GetTick();
//	led.toggle();
	}
	return ad5933.setPowerMode(POWER_STANDBY) == HAL_OK;
}

uint8_t parseData(char *str, settings *set)
{
	uint8_t mode = 0;
	if (!strcmp(str, "Connected"))
	{
		return 7;
	}
	char *rest = (char*) malloc(strlen(str));
	strcpy(rest, str);
	char *token = strtok_r(rest, " ", &rest);
	const char *commands[] =
	{ "StartSweep", "RepeatSweep", "Calibrate", "GetData", "SetParameters" };
	const char *modifiers[] =
	{ "StartFrequency", "StepFrequency", "StepCount", "RepeatCount",
			"SettlingCycles", "Averaging" };
	if (token != NULL)
	{
		for (int i = 0; i < 5; i++)
		{
			if (strcmp(token, commands[i]) == 0)
			{
				mode = i + 1;
				break;
			}
		}
		while (token != NULL && mode == 5)
		{
			token = strtok_r(NULL, " ", &rest);
			for (int i = 0; i < 6; i++)
			{
				int len = strlen(modifiers[i]);
				if (!strncmp(token, (char*) modifiers[i], len))
				{
					char *pStart = &token[len + 1];
					int val = strtol(pStart, NULL, 10);
					token = strtok_r(NULL, " ", &rest);
					switch (i)
					{
					case 0:
						set->StartFreq = val;
						break;
					case 1:
						set->IncrementFreq = val;
						break;
					case 2:
						set->IncrementCount = val;
						break;
					case 3:
						set->RepeatCount = val;
						break;
					case 4:
						set->SettlingCycles = val;
						break;
					case 5:
						set->Averaging = val;
					}
				}
			}
		}
	}
	free(rest);
	return mode;
}

bool setParameters(AD5933 &ad5933, settings set)
{

	return (ad5933.setStartFrequency(set.StartFreq) == HAL_OK
			&& ad5933.setIncrementFrequency(set.IncrementFreq) == HAL_OK
			&& ad5933.setNumberIncrements(set.IncrementCount) == HAL_OK
			&& ad5933.setNumberOfSettlingCycles(set.SettlingCycles) == HAL_OK);
}

double absoluteAngleDiffrence(double angle1, double angle2)
{
	angle1 = fmod(angle1, 2.0 * M_PI);
	if (angle1 < 0)
		angle1 += 2.0 * M_PI;
	;

	angle2 = fmod(angle2, 2.0 * M_PI);
	if (angle2 < 0)
		angle2 += 2.0 * M_PI;

	double diff = fabs(angle1 - angle2);
	return std::min(diff, 2.0 * M_PI - diff);

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
