/*
 * uart_port_plotter_interface.c
 *
 *  Created on: 13 июл. 2023 г.
 *      Author: Морской шакал
 */

#include "uart_port_plotter_interface.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAX_VALUE_LENGTH 20
#define ASCII_NUMBER_SHIFT 48

Port_plotter initPortPlotter(UART_HandleTypeDef* huart, uint8_t values_number)
{
	Port_plotter new_plotter;
	new_plotter.huart = huart;
	new_plotter.values_number = values_number;
	new_plotter.tx_buffer = malloc(sizeof(uint8_t)*(MAX_VALUE_LENGTH+1)*values_number+1);
	return new_plotter;
}
void sendMessageForPlotter(Port_plotter* plotter, ...)
{
	plotter->tx_buffer[0] = '$';
	uint32_t possition = 1;
	va_list factor;
	va_start(factor, plotter);
	for(uint8_t i = 0; i<plotter->values_number; i++)
	{
		int64_t value = va_arg(factor, int64_t);
		if(value<0)
		{
			value=-value;
			plotter->tx_buffer[possition] = '-';
			possition++;
		}
		uint32_t j = 10;
		for(;value/j;j*=10);
		j/=10;
		for(;j>0;j/=10)
		{
			plotter->tx_buffer[possition] = value / j + ASCII_NUMBER_SHIFT;
			value%=j;
			possition++;
		}
		plotter->tx_buffer[possition] = ' ';
		possition++;
	}
	possition--;
	plotter->tx_buffer[possition] = ';';
	va_end(factor);
	HAL_UART_Transmit_IT(plotter->huart, plotter->tx_buffer, possition+1);
}
