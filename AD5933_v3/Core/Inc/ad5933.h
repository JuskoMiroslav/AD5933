/*
 * ad5933.h
 *
 *  Created on: Nov 5, 2024
 *      Author: Miroslav Jusko
 */

#ifndef INC_AD5933_H_
#define INC_AD5933_H_

#include "main.h"

#define AD5933_ADDR			(0x0D<<1)
#define OSC_FREQ			(16000000U)

#define CTRL_REG1   		(0x80)
#define CTRL_REG2			(0x81)
#define STATUS_REG			(0x8F)
#define TEMP_DATA_1     	(0x92)
#define TEMP_DATA_2     	(0x93)

#define START_FREQ_1    	(0x82)
#define START_FREQ_2    	(0x83)
#define START_FREQ_3   		(0x84)

#define INC_FREQ_1      	(0x85)
#define INC_FREQ_2      	(0x86)
#define INC_FREQ_3      	(0x87)

#define NUM_INC_1       	(0x88)
#define NUM_INC_2       	(0x89)

#define NUM_SCYCLES_1	   	(0x8A)
#define NUM_SCYCLES_2   	(0x8B)

#define REAL_DATA_1     	(0x94)
#define REAL_DATA_2     	(0x95)

#define IMAG_DATA_1     	(0x96)
#define IMAG_DATA_2     	(0x97)

#define CTRL_RESET			(0b00010000)
#define CTRL_TEMP_MEASURE	(0b10010000)
#define CTRL_CLOCK_EXTERNAL (0b00001000)
#define CTRL_CLOCK_INTERNAL	(0b00000000)

#define CTRL_POWER_DWN_MODE (0b10100000)
#define CTRL_STANDBY_MODE   (0b10110000)
#define CTRL_NO_OPERATION   (0b00000000)
#define CTRL_INIT_START_F	(0b00010000)
#define CTRL_START_F_SWEEP	(0b00100000)
#define CTRL_INCREMENT_F	(0b00110000)
#define CTRL_REPEAT_FREQ    (0b01000000)

#define STATUS_TEMP_VALID	(0x01)
#define STATUS_DATA_VALID	(0x02)
#define STATUS_SWEEP_DONE	(0x04)

typedef enum{
POWER_STANDBY = CTRL_STANDBY_MODE,
POWER_DOWN = CTRL_POWER_DWN_MODE,
POWER_ON = CTRL_NO_OPERATION
}HAL_PowerMode;

typedef enum{
OUTPUT_RANGE_1 = (0b00000000),
OUTPUT_RANGE_2 = (0b00000110),
OUTPUT_RANGE_3 = (0b00000100),
OUTPUT_RANGE_4 = (0b00000010)
}HAL_OutputRange;
class AD5933{
public:
	AD5933(I2C_HandleTypeDef *hi2c);
	~AD5933();
	uint8_t init(uint32_t StartFreq,uint32_t IncrementFreq,uint32_t IncrementCount,HAL_OutputRange OutRange,bool ExternalClock);
	uint8_t init(uint32_t StartFreq,uint32_t IncrementFreq,uint32_t IncrementCount,HAL_OutputRange OutRange);

	HAL_StatusTypeDef setControlRegister(uint8_t mode);
	HAL_StatusTypeDef reset(void);
	HAL_StatusTypeDef setExternalClockSource(bool source);
	HAL_StatusTypeDef setStartFrequency(uint32_t start);
	HAL_StatusTypeDef setIncrementFrequency(uint32_t increment);
	HAL_StatusTypeDef setNumberOfSettlingCycles(uint32_t cycles);
	HAL_StatusTypeDef setNumberIncrements(uint16_t num);
	HAL_StatusTypeDef setPGAGain(bool gain);
	HAL_StatusTypeDef setPowerMode(HAL_PowerMode level);
	HAL_StatusTypeDef setOutputRange(HAL_OutputRange range);
	HAL_StatusTypeDef getComplexData(int16_t *real,int16_t *imag);
	HAL_StatusTypeDef frequencySweep(int16_t real[],int16_t imag[],int n);
	HAL_StatusTypeDef calibrate(double gain[],int ref,int n);

	uint8_t readRegister(uint8_t reg);


	double getTemp();
protected:
	I2C_HandleTypeDef  *_hi2c;
private:
	static HAL_StatusTypeDef readByte(I2C_HandleTypeDef *hi2c, uint8_t reg,uint8_t *data);
	static HAL_StatusTypeDef writeByte(I2C_HandleTypeDef *hi2c, uint8_t reg,uint8_t data);
};

#endif /* INC_AD5933_H_ */
