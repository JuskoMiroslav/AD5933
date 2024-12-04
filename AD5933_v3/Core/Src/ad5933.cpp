/*
 * ad5933.c
 *
 *  Created on: Nov 5, 2024
 *      Author: Miroslav Jusko
 */

#include "ad5933.h"
#include "math.h"

AD5933::AD5933(I2C_HandleTypeDef *hi2c):_hi2c(hi2c){
}

HAL_StatusTypeDef AD5933::readByte(I2C_HandleTypeDef *hi2c, uint8_t reg,uint8_t *data){
	uint8_t adr[1] = {0xB0};
	HAL_StatusTypeDef out = HAL_OK;
	out = HAL_I2C_Master_Transmit(hi2c, AD5933_ADDR, &reg, 1,1000);
	if(out != HAL_OK)
		return HAL_ERROR;
	out = HAL_I2C_Master_Receive(hi2c, AD5933_ADDR, adr, 1, 1000);
	*data = adr[0];
	return out;
//	do{
//				out = HAL_I2C_Mem_Write(hi2c, AD5933_ADDR, 0xB0, I2C_MEMADD_SIZE_8BIT, &reg, 1, HAL_MAX_DELAY);
//	}while (out == HAL_BUSY);
//	uint8_t uData = 0;
//	HAL_Delay(1);
//	out = HAL_I2C_Master_Receive(hi2c, AD5933_ADDR, b, 1, HAL_MAX_DELAY);
//	do{
//	out = HAL_I2C_Mem_Read(hi2c, AD5933_ADDR, reg, 1, &uData, 1, 1000);
//	}while (out == HAL_BUSY);
//	*data = uData;
//	return out;

}


uint8_t AD5933::init(uint32_t StartFreq,uint32_t IncrementFreq,uint32_t IncrementCount,HAL_OutputRange OutRange, bool ExternalClock = true){
	if(reset() != HAL_OK)
		return 1;
	if(setExternalClockSource(ExternalClock) != HAL_OK)
		return 2;
	if(setStartFrequency(StartFreq) != HAL_OK)
		return 3;
	if(setIncrementFrequency(IncrementFreq) != HAL_OK)
		return 4;
	if(setNumberIncrements(IncrementCount) != HAL_OK)
		return 5;
	if(setOutputRange(OutRange)!= HAL_OK)
		return 6;
	return 0;

}

uint8_t AD5933::init(uint32_t StartFreq,uint32_t IncrementFreq,uint32_t IncrementCount,HAL_OutputRange OutRange){
	return init(StartFreq,IncrementFreq,IncrementCount,OutRange,true);
}

HAL_StatusTypeDef AD5933::writeByte(I2C_HandleTypeDef *hi2c, uint8_t reg,uint8_t data){
	return HAL_I2C_Mem_Write(hi2c, AD5933_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
}


uint8_t AD5933::readRegister(uint8_t reg){
	uint8_t val;
	if(readByte(_hi2c,reg, &val) != HAL_OK)
		return 0xff;
	return val;
}

HAL_StatusTypeDef AD5933::setControlRegister(uint8_t mode){
	uint8_t val =0;
	if(readByte(_hi2c, CTRL_REG1, &val) != HAL_OK)
		return HAL_ERROR;
	val &= 0x0f;
	val |= mode;
	return writeByte(_hi2c, CTRL_REG1, val);
}

HAL_StatusTypeDef AD5933::reset(){
	uint8_t val = 0;
	if(readByte(_hi2c, CTRL_REG2, &val) != HAL_OK)
		return HAL_ERROR;
	val |= CTRL_RESET;
	return writeByte(_hi2c, CTRL_REG2, val);
}

HAL_StatusTypeDef AD5933::setExternalClockSource(bool source){
	if(source)
		return writeByte(_hi2c, CTRL_REG2, CTRL_CLOCK_EXTERNAL);
	else
		return writeByte(_hi2c, CTRL_REG2, CTRL_CLOCK_INTERNAL);
}

double AD5933::getTemp(){

	setControlRegister(CTRL_TEMP_MEASURE);
	while((readRegister(STATUS_REG) &STATUS_TEMP_VALID) != STATUS_TEMP_VALID);
	uint8_t rawTemp[2] = {0,0};
	if(readByte(_hi2c, TEMP_DATA_1, &rawTemp[0]) == HAL_OK &&
			readByte(_hi2c, TEMP_DATA_2, &rawTemp[1]) == HAL_OK)
	{
		uint16_t rawTempVal = (rawTemp[0] <<8 | rawTemp[1]) & 0x1fff;
		if((rawTemp[0] & (1<<5)) == 0)
			return rawTempVal / 32.0;
		return (rawTempVal - 16384) /32;
	}
	return -1;
}

HAL_StatusTypeDef AD5933::setStartFrequency(uint32_t start){
	float freq = (start/(OSC_FREQ/4.0))*pow(2,27);
	uint32_t freqHex = freq;
	if(freqHex > 0xFFFFFF)
		return HAL_ERROR;
	uint8_t highByte= (freqHex >> 16) & 0xff;
	uint8_t midByte= (freqHex >> 8) & 0xff;
	uint8_t lowByte= freqHex & 0xff;
	if(	writeByte(_hi2c, START_FREQ_1, highByte) != HAL_OK ||
		writeByte(_hi2c, START_FREQ_2, midByte) != HAL_OK ||
		writeByte(_hi2c, START_FREQ_3, lowByte) != HAL_OK)
		return HAL_ERROR;
	return HAL_OK;
}

HAL_StatusTypeDef AD5933::setIncrementFrequency(uint32_t increment){
	float freq = (increment/(OSC_FREQ/4.0))*pow(2,27);
	uint32_t freqHex = freq;
	if(freqHex > 0xFFFFFF)
		return HAL_ERROR;
	uint8_t highByte= (freqHex >> 16) & 0xff;
	uint8_t midByte= (freqHex >> 8) & 0xff;
	uint8_t lowByte= freqHex & 0xff;
	if(	writeByte(_hi2c, INC_FREQ_1, highByte) != HAL_OK ||
		writeByte(_hi2c, INC_FREQ_2, midByte) != HAL_OK ||
		writeByte(_hi2c, INC_FREQ_3, lowByte) != HAL_OK)
		return HAL_ERROR;
	return HAL_OK;
}

HAL_StatusTypeDef AD5933::setNumberIncrements(uint16_t num){
	if(num > 0x1FF)
		return HAL_ERROR;
	uint8_t highByte= (num >> 8) & 0xff;
	uint8_t lowByte= num & 0xff;
	if(	writeByte(_hi2c, NUM_INC_1, highByte) != HAL_OK ||
		writeByte(_hi2c, NUM_INC_2, lowByte) != HAL_OK)
		return HAL_ERROR;
	return HAL_OK;
}
HAL_StatusTypeDef AD5933::setNumberOfSettlingCycles(uint32_t cycles){

	uint8_t settleTime[2];
	if(cycles >1023)
	{
		settleTime[0] = (cycles >> 2) & 0xff;
		settleTime[1] = ((cycles >> 10) & 0x1) | 0x6;
	}
	else if(cycles >511)
	{
		settleTime[0] = (cycles >> 1) & 0xff;
		settleTime[1] = ((cycles >> 9) & 0x1) | 0x2;
	}
	else
	{
		settleTime[0] = cycles & 0xff;
		settleTime[1] = (cycles >> 8) & 0x1;
	}
	if(writeByte(_hi2c,NUM_SCYCLES_1,settleTime[0]) != HAL_OK ||
	   writeByte(_hi2c,NUM_SCYCLES_2,settleTime[1]) != HAL_OK)
		return HAL_ERROR;
	return HAL_OK;
}

HAL_StatusTypeDef AD5933::setPGAGain(bool gain){
	uint8_t val;
	if(readByte(_hi2c, CTRL_REG1, &val) != HAL_OK)
		return HAL_ERROR;
	val &= 0xFE;
	if(!gain)
		val |= 0x1;
	return writeByte(_hi2c, CTRL_REG1, val);
}


HAL_StatusTypeDef AD5933::setPowerMode(HAL_PowerMode level){
	HAL_StatusTypeDef res;
	switch(level){
	case POWER_ON:
		res = setControlRegister(CTRL_NO_OPERATION);
		break;
	case POWER_STANDBY:
		res = setControlRegister(CTRL_STANDBY_MODE);
  		break;
	case POWER_DOWN:
		res = setControlRegister(CTRL_POWER_DWN_MODE);
		break;
	default:
		return HAL_ERROR;
	}
	return res;
}

HAL_StatusTypeDef AD5933::setOutputRange(HAL_OutputRange range){
	uint8_t val;
	if(readByte(_hi2c, CTRL_REG1, &val) != HAL_OK)
		return HAL_ERROR;
	val &= 0xF9;
	switch (range) {
		case OUTPUT_RANGE_2:
			val |= OUTPUT_RANGE_2;
			break;
		case OUTPUT_RANGE_3:
			val |= OUTPUT_RANGE_3;
			break;
		case OUTPUT_RANGE_4:
			val |= OUTPUT_RANGE_4;
			break;
		default:
			val |= OUTPUT_RANGE_1;
			break;
	}
	return writeByte(_hi2c, CTRL_REG1, val);
}

HAL_StatusTypeDef AD5933::getComplexData(int16_t *real,int16_t *imag){
//	while((readRegister(STATUS_REG) &STATUS_DATA_VALID) != STATUS_DATA_VALID);
	uint8_t realComp[2];
	uint8_t imagComp[2];
	if( readByte(_hi2c, REAL_DATA_1, &realComp[0]) == HAL_OK &&
		readByte(_hi2c, REAL_DATA_2, &realComp[1]) == HAL_OK &&
		readByte(_hi2c, IMAG_DATA_1, &imagComp[0]) == HAL_OK &&
		readByte(_hi2c, IMAG_DATA_2, &imagComp[1]) == HAL_OK){
		*real = (int)(((realComp[0] << 8) | realComp[1]) & 0xFFFF);
		*imag = (int)(((imagComp[0] << 8) | imagComp[1]) & 0xFFFF);
		return HAL_OK;
	}
	else{
		*real = -1;
		*imag = -1;
		return HAL_ERROR;
	}
}
HAL_StatusTypeDef AD5933::frequencySweep(int16_t real[],int16_t imag[],int n){
	if(!(setPowerMode(POWER_STANDBY) == HAL_OK &&
	setControlRegister(CTRL_INIT_START_F) == HAL_OK &&
	setControlRegister(CTRL_START_F_SWEEP) == HAL_OK))
		return HAL_ERROR;
	int i = 0;


//	HAL_GPIO_WritePin(sync_GPIO_Port, sync_Pin, GPIO_PIN_SET);
	while((readRegister(STATUS_REG) &STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE){
		if(getComplexData(&real[i], &imag[i]) != HAL_OK)
			return HAL_ERROR;
		if(i >= n)
			return HAL_ERROR;

//		setControlRegister(CTRL_REPEAT_FREQ);

		i++;
		setControlRegister(CTRL_INCREMENT_F);
		while((readRegister(STATUS_REG) &STATUS_DATA_VALID) != STATUS_DATA_VALID);

	}
//		HAL_Delay(5000);
//	/(sync_GPIO_Port, sync_Pin, GPIO_PIN_RESET);
	return setPowerMode(POWER_STANDBY);
}



HAL_StatusTypeDef AD5933::calibrate(double gain[],int ref,int n){
	int16_t* real = (int16_t*)malloc(n*sizeof(int16_t));
	int16_t* imag = (int16_t*)malloc(n*sizeof(int16_t));
	if(frequencySweep(real, imag, n)!=HAL_OK)
	{
		free(real);
		free(imag);
		return HAL_ERROR;
	}
	for(int i = 0;i<n;i++)
	{
		gain[i] = (double)(1.0/ref)/sqrt(pow(real[i],2)+pow(imag[i],2));
	}

	free(real);
	free(imag);
	return HAL_OK;
}
