/*
 * uart_port_plotter_interface.h
 *
 *  Created on: 13 июл. 2023 г.
 *      Author: Морской шакал
 */

#ifndef INC_UART_PORT_PLOTTER_INTERFACE_H_
#define INC_UART_PORT_PLOTTER_INTERFACE_H_

#include "stm32f1xx_hal.h"

#include <stdint.h>

typedef struct
{
	UART_HandleTypeDef* huart;
	uint8_t values_number;
	uint8_t* tx_buffer;
} Port_plotter;

Port_plotter initPortPlotter(UART_HandleTypeDef* huart, uint8_t values_number);
void sendMessageForPlotter(Port_plotter* plotter, ...);

#endif /* INC_UART_PORT_PLOTTER_INTERFACE_H_ */
