/*
 * filter.c
 *
 *  Created on: 24 июн. 2023 г.
 *      Author: Морской шакал
 */
#include "moving_average.h"
#include <stdlib.h>

Moving_Average initFilter(uint16_t filter_order)
{
	Moving_Average new_filter;
	new_filter.filter_order = filter_order;
	new_filter.values = malloc(sizeof(uint32_t)*filter_order);
	for(uint16_t i = 0; i<filter_order; i++)
		new_filter.values[i] = 0;
	new_filter.next_value = 0;
	new_filter.sum = 0;
	return new_filter;
}
uint8_t addValue(Moving_Average* filter, int64_t value)
{
	filter->sum+=value;
	filter->sum-=filter->values[filter->next_value];
	filter->values[filter->next_value]=value;
	filter->next_value++;
	filter->next_value%=filter->filter_order;
	return 0;
}
int64_t getFiltred(Moving_Average* filter)
{
	return filter->sum/filter->filter_order;
}
