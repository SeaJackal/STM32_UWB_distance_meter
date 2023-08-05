/*
 * filter.h
 *
 *  Created on: 24 июн. 2023 г.
 *      Author: Морской шакал
 */

#ifndef INC_MOVING_AVERAGE_H_
#define INC_MOVING_AVERAGE_H_

#include <stdint.h>

typedef struct
{
	uint16_t filter_order;
	int64_t* values;
	uint16_t next_value;
	int64_t sum;
} Moving_Average;

Moving_Average initFilter(uint16_t filter_order);
uint8_t addValue(Moving_Average* filter, int64_t value);
int64_t getFiltred(Moving_Average* filter);

#endif /* INC_MOVING_AVERAGE_H_ */
