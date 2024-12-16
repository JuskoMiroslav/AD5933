#ifndef INC_GPIO_HAL_H_
#define INC_GPIO_HAL_H_

#include "gpio.h"

class GPIObase
{
public:
	GPIObase(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin);
	~GPIObase();

	static void init(void);

	GPIO_PinState read(void);
protected:
	GPIO_TypeDef* _GPIOx;
	uint16_t _GPIO_Pin;
private:
	static bool isInit;
};

class Dout : public GPIObase
{
public:
	Dout(GPIO_TypeDef* GPIOx,
		 uint16_t GPIO_Pin,
		 GPIO_PinState PinState = GPIO_PIN_SET);
	~Dout();

	void write(GPIO_PinState state);
	void toggle(void);
	GPIO_PinState read(void);

private:

};

class Din : public GPIObase
{
public:
	Din(GPIO_TypeDef* GPIOx,
			 uint16_t GPIO_Pin);
	~Din();
	GPIO_PinState read(void);
private:

};
#endif /* INC_GPIO_HAL_H_ */


