/*
 * dma_hal.h
 *
 *  Created on: Oct 30, 2024
 *      Author: Miroslav Jusko
 */

#ifndef INC_DMA_HAL_H_
#define INC_DMA_HAL_H_
#include "dma.h"

class DMA{
public:
	DMA();
	static void init(void);
private:
	static bool is_init;
};



#endif /* INC_DMA_HAL_H_ */
