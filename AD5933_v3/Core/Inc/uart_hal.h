/*
 * uart_hal.h
 *
 *  Created on: Oct 30, 2024
 *      Author: Miroslav Jusko
 */

#ifndef INC_UART_HAL_H_
#define INC_UART_HAL_H_

#include "main.h"
#include "usart.h"
#include "ISR_HAL.h"
#include "CircularBuffer.h"
#include "dma_hal.h"

#define UART_count (1)
#define RX_SIZE_BUFFER (64)

class UARTbase{
public:
	UARTbase(USART_TypeDef* instance, UART_HandleTypeDef* huart);
	~UARTbase();

	void init();

	HAL_StatusTypeDef write(uint8_t *pData, uint16_t Size, uint32_t Timeout = 1000);
	HAL_StatusTypeDef read(uint8_t *pData, uint16_t Size, uint32_t Timeout = 1000);
protected:
	USART_TypeDef* _Instance;
	UART_HandleTypeDef* _huart;
private:
	static bool is_init[UART_count];
};

class UartIT : public UARTbase{
public:
	UartIT(USART_TypeDef* instance, UART_HandleTypeDef* huart);
	~UartIT(void);

	virtual HAL_StatusTypeDef write(uint8_t *pData, uint16_t Size);
	virtual HAL_StatusTypeDef start_read(void);
	uint16_t read(uint8_t *pData, uint16_t Size);
	bool is_tx_done(void);
	uint16_t avalible(void);
protected:
	bool _is_tx_done;
	uint8_t _read_buffer[RX_SIZE_BUFFER];
	CircularBuffer <uint8_t> *_buffer;
	static ISR<UartIT> ISR_List;

private:
	static void TxCpltCallback(UART_HandleTypeDef *huart);
	static void RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos);

	virtual void put(uint16_t index, uint16_t size);
};


class UartDMA: public UartIT{
public:
	UartDMA(USART_TypeDef* instance, UART_HandleTypeDef* huart);
	~UartDMA(void);

	virtual HAL_StatusTypeDef write(uint8_t *pData, uint16_t Size) override;
	virtual HAL_StatusTypeDef start_read(void) override;
private:
	virtual void put(uint16_t index, uint16_t size) override;

};

#endif /* INC_UART_HAL_H_ */
