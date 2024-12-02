/*
 * uart_hal.cpp
 *
 *  Created on: Oct 30, 2024
 *      Author: Miroslav Jusko
 */
#include "uart_hal.h"

bool UARTbase::is_init[] = {false};
ISR<UartIT> UartIT::ISR_List;

void UartIT::TxCpltCallback(UART_HandleTypeDef *huart)
{
	for(uint16_t i = 0; i< ISR_List.size();i++)
	{
		if(ISR_List.get(i)->_huart == huart)
			ISR_List.get(i) -> _is_tx_done = true;
	}
}

void UartIT::RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos){
	for(uint16_t i = 0; i< ISR_List.size();i++)
	{
		if(ISR_List.get(i)->_huart == huart)
			ISR_List.get(i) -> put(0,Pos);
	}
}

UARTbase::UARTbase(USART_TypeDef* instance, UART_HandleTypeDef* huart)
	:_Instance(instance),_huart(huart)
{
	init();
}

UARTbase::~UARTbase()
{
	if(_Instance == USART2){
		if(!is_init[0])
			return;
	}
	is_init[0]= false;
	HAL_UART_MspDeInit(_huart);

}

void UARTbase::init()
{
	if(_Instance==USART2)
	{
		if(is_init[0])
			return;
		is_init[0]=true;
		MX_USART2_UART_Init();
	}
}

HAL_StatusTypeDef UARTbase::write(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return HAL_UART_Transmit(_huart,pData,Size,Timeout);
}

HAL_StatusTypeDef UARTbase::read(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return HAL_UART_Receive(_huart,pData,Size,Timeout);
}

UartIT::UartIT(USART_TypeDef* instance, UART_HandleTypeDef* huart)
	:UARTbase(instance,huart),_is_tx_done(true)
{
#if USE_HAL_UART_REGISTER_CALLBACKS != 1
#error "USE_HAL_UART_REGISTER_CALLBACKS must be set to 1 in stm32f4xx_hal_conf.h"
#endif
	_buffer = new CircularBuffer<uint8_t>(sizeof(_read_buffer)*2);
	huart->TxCpltCallback = UartIT::TxCpltCallback;
	huart->RxEventCallback = UartIT::RxEventCallback;
	ISR_List.add(this);
}

UartIT::~UartIT(void)
{
	if(_buffer != NULL)
		delete _buffer;
	ISR_List.remove(this);
}

HAL_StatusTypeDef UartIT::write(uint8_t *pData, uint16_t Size)
{
	if(!_is_tx_done)
		return HAL_BUSY;
	_is_tx_done = false;
	return HAL_UART_Transmit_IT(_huart, pData,Size);
}

uint16_t UartIT::read(uint8_t *pData, uint16_t Size)
{
	return _buffer->pull(pData,Size);
}

HAL_StatusTypeDef UartIT::start_read(void)
{
	memset(_read_buffer,0,sizeof(_read_buffer));
	return HAL_UARTEx_ReceiveToIdle_IT(_huart,_read_buffer,sizeof(_read_buffer));
}

void UartIT::put(uint16_t index, uint16_t size)
{
	_buffer->put(&_read_buffer[index],size);
	start_read();
}
bool UartIT::is_tx_done(void)
{
	return _is_tx_done;
}

uint16_t UartIT::avalible(){
	return _buffer->count();
}
UartDMA::UartDMA(USART_TypeDef* instance, UART_HandleTypeDef* huart)
	:UartIT(instance,huart)
{
	DMA::init();
	ISR_List.add(this);
}

UartDMA::~UartDMA(void)
{
	ISR_List.remove(this);
}

HAL_StatusTypeDef UartDMA::write(uint8_t *pData, uint16_t Size)
{
	if(!_is_tx_done)
			return HAL_BUSY;
	_is_tx_done = false;
	HAL_StatusTypeDef ret= HAL_OK;

	ret = HAL_UART_Transmit_DMA(_huart, pData,Size);
	if(ret != HAL_OK){
		_is_tx_done = true;
	}

	return ret;
}
HAL_StatusTypeDef UartDMA::start_read(void)
{
	memset(_read_buffer, 0, RX_SIZE_BUFFER);
	return HAL_UARTEx_ReceiveToIdle_DMA(_huart,_read_buffer,RX_SIZE_BUFFER);
}

void UartDMA::put(uint16_t index, uint16_t Size)
{
	static uint16_t last_size(0);
	static uint16_t next_index(0);
	uint16_t size = Size - last_size;

	_buffer->put(&_read_buffer[next_index],size);
	next_index += size;
	if(next_index >= RX_SIZE_BUFFER)
		next_index -=RX_SIZE_BUFFER;
	if(last_size >= RX_SIZE_BUFFER)
	{
		last_size = 0;
		return;
	}
	last_size = Size;
}

