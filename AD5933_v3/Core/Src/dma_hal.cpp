/*
 * dma_hal.cpp
 *
 *  Created on: Oct 30, 2024
 *      Author: Miroslav Jusko
 */


#include "dma_hal.h"

bool DMA::is_init = false;

DMA::DMA()
{

}
void DMA::init(void)
{
	if(is_init)
		return;
	DMA::is_init=true;
	MX_DMA_Init();
}

