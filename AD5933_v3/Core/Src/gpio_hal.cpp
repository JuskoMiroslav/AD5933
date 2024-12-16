#include "gpio_hal.h"

bool GPIObase::isInit = false;

GPIObase::GPIObase(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)
				  : _GPIOx(GPIOx),_GPIO_Pin(GPIO_Pin)
{
	GPIObase::init();
}

GPIObase::~GPIObase()
{

}

void GPIObase::init(void)
{
	if(GPIObase::isInit)
		return;
	MX_GPIO_Init();
	GPIObase::isInit = true;
}

GPIO_PinState GPIObase::read(void)
{
	return HAL_GPIO_ReadPin(_GPIOx, _GPIO_Pin);
}

Dout::Dout(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state)
		  : GPIObase(GPIOx,GPIO_Pin)
{
	write(state);
}

Dout::~Dout()
{

}
void Dout::write(GPIO_PinState state)
{
	HAL_GPIO_WritePin(_GPIOx, _GPIO_Pin, state);
}

void Dout::toggle(void)
{
	HAL_GPIO_TogglePin(_GPIOx, _GPIO_Pin);
}

GPIO_PinState Dout::read(void)
{
	return HAL_GPIO_ReadPin(_GPIOx, _GPIO_Pin);
}


Din::Din(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
		: GPIObase(GPIOx, GPIO_Pin)
{

}
Din::~Din()
{

}
GPIO_PinState Din::read(void)
{
	return HAL_GPIO_ReadPin(_GPIOx, _GPIO_Pin);
}

