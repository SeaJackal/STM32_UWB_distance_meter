#ifndef FREE_INDEX_CTRL_H
#define FREE_INDEX_CTRL_H

#include <stdint.h>

typedef struct
{
	const uint8_t number;
	
	uint8_t* indexes;
	uint8_t next_possition;
}Free_ctrl;

Free_ctrl Free_ctrl_init(uint8_t number);

uint8_t Free_ctrl_unfreeIndex(Free_ctrl* ctrl);
void Free_ctrl_freeIndex(Free_ctrl* ctrl, uint8_t index);
uint8_t Free_ctrl_isEmpty(Free_ctrl* ctrl);
uint8_t Free_ctrl_isFull(Free_ctrl* ctrl);

#endif