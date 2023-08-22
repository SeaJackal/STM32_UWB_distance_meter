#include "free_index_controller.h"

#include <stdlib.h>

Free_ctrl Free_ctrl_init(uint8_t number)
{
	uint8_t* indexes = malloc(sizeof(uint8_t) * number);
	for(uint8_t i = 0; i<number; i++)
		indexes[i] = number-1-i;
	Free_ctrl new_ctrl = {
		.number = number,
		.indexes = indexes,
		.next_possition = number
	};
	return new_ctrl;
}
uint8_t Free_ctrl_unfreeIndex(Free_ctrl* ctrl)
{
	ctrl->next_possition--;
	return ctrl->indexes[ctrl->next_possition];
}
void Free_ctrl_freeIndex(Free_ctrl* ctrl, uint8_t index)
{
	ctrl->indexes[ctrl->next_possition] = index;
	ctrl->next_possition++;
}
uint8_t Free_ctrl_isEmpty(Free_ctrl* ctrl)
{
	return ctrl->next_possition==0;
}
uint8_t Free_ctrl_isFull(Free_ctrl* ctrl)
{
	return ctrl->next_possition==ctrl->number;
}