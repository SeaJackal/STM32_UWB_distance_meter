/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   TX then wait for response example code
 *
 *           This is a simple code example that sends a frame and then turns on the DW1000 receiver to receive a response. The response could be
 *           anything as no check is performed on it and it is only displayed in a local buffer but the sent frame is the one expected by the
 *           companion simple example "RX then send a response example code". After the response is received or if the reception timeouts, this code
 *           just go back to the sending of a new frame.
 *
 * @attention
 *
 * Copyright 2015 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include "deca_device_api.h"
#include "deca_regs.h"
#include "sleep.h"
#include "dw1000.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "uart_port_plotter_interface.h"
#include "uwb_ui.h"
//#include "uwb_simple_distance_meter.h"
#include "uwb_distance_meter.h"
#include "EventRecorder.h"
#include "moving_average.h"

/* Example application name and version to display on LCD screen. */
#define APP_NAME "TX WAITRESP v1.2"
#define UART_MESSAGE_LEN 30

#define ERROR_TIMEOUT 1
#define COUNT_DISTANCE(time) (time-b)*100/k

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */

uint32_t signal_message;
RepeaterMessage responce_message;

uint8_t uart_flag = 0;
uint8_t uart_command;

uint64_t time_1;
uint64_t time_2;

uint64_t k = 200;
uint64_t b = 32800;

uint32_t distance = 0;

//void start_agent()
//{
//	Port_plotter plotter = initPortPlotter(&huart1, 1);
//	UWBS_Agent agent;
//	Moving_Average filter = initFilter(100);
//	HAL_UART_Receive_IT(&huart1, &uart_command, 1);
//	while(UWBS_initAgent(&agent)!=UWBS_OK)
//	{
//		printf("Init error\n");
//		HAL_Delay(UWBS_ERROR_TIMEOUT);
//	}
//	printf("Init OK\n");
//	while(1)
//	{
//		//EventRecord2(0, 0, 1);
//		while(UWBS_sendMessage(&agent)!=UWBS_OK)
//		{
//			printf("Transmition error\n");
//			HAL_Delay(UWBS_ERROR_TIMEOUT);
//			EventRecord2(0, 0, 6);
//		}
//		printf("Transmition OK\n");
//		//EventRecord2(0, 0, 2);
//		if(uart_flag)
//		{
//			if(uart_command == '1')
//				time_1 = getFiltred(&filter);
//			else if(uart_command == '2')
//				time_2 = getFiltred(&filter);
//			else if(uart_command == '3')
//			{
//				k = time_2 - time_1;
//				b = time_1 - k;
//			}
//			uart_flag = 0;
//			HAL_UART_Receive_IT(&huart1, &uart_command, 1);
//		}
//		switch(UWBS_getMessage(&agent))
//		{
//			case UWBS_DW_ERROR:
//				EventRecord2(0, 0, 3);
//				printf("Receiving error\n");
//				HAL_Delay(UWBS_ERROR_TIMEOUT);
//				break;
//			case UWBS_RESPONDER_ERROR:
//				EventRecord2(0, 0, 4);
//				printf("Connection error\n");
//				break;
//			case UWBS_OK:
//				EventRecord2(0, 0, 5);
//				printf("Receiving OK\n");
//				//printf("Own:%lli; Received:%lli\n", agent.self_time, agent.received_time);
//				printf("Own:%lli; Received:%lli; Filtred:%lli\n", agent.self_time, agent.received_time, getFiltred(&filter));
//				distance = (agent.received_time-b)*100/k;
//				if(distance < 3000)
//					addValue(&filter, distance);
//				distance = (agent.self_time-b)*100/k;
//				if(distance < 3000)
//					addValue(&filter, distance);
//				sendMessageForPlotter(&plotter, getFiltred(&filter));
//				break;
//			default:
//				EventRecord2(0, 0, 7);
//				break;
//		}
//		//HAL_Delay(5);
//	}
//}

void start_dm_agent()
{
	Port_plotter plotter = initPortPlotter(&huart1, 4);
	//Moving_Average filter[4];
	//for(uint8_t i = 0; i<4; i++)
		//filter[i]=initFilter(2);
	UWB_DM_Agent agent;
	while(UWB_DM_init(&agent, 4, 0)!=UWB_DM_OK)
	{
		printf("Init error\n");
		HAL_Delay(ERROR_TIMEOUT);
	}
	printf("Init successful\n");
	while(1)
	{
		uint8_t no_error_flag = 1;
		printf("Connection searching\n");
		UWB_DM_Status status = UWB_DM_iterate(&agent);
		while(status==UWB_DM_DW_ERROR)
		{
			printf("DW error\n");
			HAL_Delay(ERROR_TIMEOUT);
			while(UWB_DM_reset(&agent)==UWB_DM_DW_ERROR)
			{
				printf("Reset error\n");
				HAL_Delay(ERROR_TIMEOUT);
			}
			printf("Reset done\n");
			status = UWB_DM_iterate(&agent);
		}
		if(status == UWB_DM_CONNECTION_ERROR)
			printf("Connection failed\n");
		while(no_error_flag)
		{
			switch(agent.state)
			{
				case UWB_DM_WAITING_MESSAGE:
					printf("Waiting message\n");
					break;
				case UWB_DM_READY_TO_SEND:
					printf("Ready to send\n");
					break;
				default:
					printf("Code error\n");
					break;
			}
			switch(UWB_DM_iterate(&agent))
			{
				case UWB_DM_DW_ERROR:
					printf("DW error\n");
					HAL_Delay(ERROR_TIMEOUT);
					while(UWB_DM_reset(&agent)==UWB_DM_DW_ERROR)
					{
						printf("Reset error\n");
						HAL_Delay(ERROR_TIMEOUT);
					}
					printf("Reset done\n");
					no_error_flag = 0;
					break;
				case UWB_DM_CONNECTION_ERROR:
					printf("Connection error\n");
					while(UWB_DM_reset(&agent)==UWB_DM_DW_ERROR)
					{
						printf("Reset error\n");
						HAL_Delay(ERROR_TIMEOUT);
					}
					HAL_Delay(ERROR_TIMEOUT);
					no_error_flag = 0;
					break;
				case UWB_DM_OK:
					printf("Iteration OK\n");
					for(uint8_t i = 0; i<4; i++)
					{
						if(i == agent.self_index)
							continue;
						printf("Agent %i:%i %llu %llu\n",
							i, agent.connection_bits&1<<i, agent.self_times[i], agent.received_times[i]);
					}
					sendMessageForPlotter(&plotter, 
						(COUNT_DISTANCE(agent.self_times[0])<3000)?COUNT_DISTANCE(agent.self_times[0]):0,
						(COUNT_DISTANCE(agent.self_times[1])<3000)?COUNT_DISTANCE(agent.self_times[1]):0,
						(COUNT_DISTANCE(agent.self_times[2])<3000)?COUNT_DISTANCE(agent.self_times[2]):0,
						(COUNT_DISTANCE(agent.self_times[3])<3000)?COUNT_DISTANCE(agent.self_times[3]):0);
//					for(uint8_t i = 0; i<4; i++)
//					{
//						uint32_t distance = COUNT_DISTANCE(agent.self_times[i]);
//						if(distance < 3000)
//							addValue(filter+i, distance);
//					}
//					sendMessageForPlotter(&plotter, 
//						getFiltred(filter),
//						getFiltred(filter+1),
//						getFiltred(filter+2),
//						getFiltred(filter+3));
					break;
			}
		}
	}
}

void start_repeater()
{
	UWB_Init(0);
	uint64_t rx_time = 0;
	uint64_t tx_time = 0;
	uint64_t sys_time_1 = 0;
	uint64_t sys_time_2 = 0;
	UWB_ActivateRX();
	while(1)
	{
		signal_message = 10;
		UWB_status status = UWB_WaitForMessage(&signal_message, sizeof(uint8_t));
		if(status==UWB_ERROR)
		{
			//spi_set_rate_low();
			//dwt_softreset();
			//spi_set_rate_high();
			HAL_Delay(500);
		}
		responce_message.sign = *(uint8_t*)(&signal_message);
		UWB_SendMessage(&responce_message, sizeof(RepeaterMessage), 1);
		//printf("%i %i %lli\n", responce_message.sign, responce_message.correction_sign, responce_message.time_correction);
		responce_message.correction_sign = signal_message;
		rx_time = UWB_GetRxTimestamp64();
		tx_time = UWB_GetTxTimestamp64();
		responce_message.time_correction = (tx_time>=rx_time)?(tx_time-rx_time):(tx_time+(1ll<<40)-rx_time);
		//responce_message.time_correction = (tx_time-rx_time);
	}
}

void start_distance_meter()
{
	UWB_Init(10000);
	uint64_t tx_time[10];
	for(uint8_t i = 0; i<10; i++)
		tx_time[i] = 0;
	uint64_t rx_time = 0;
	uint64_t delta_time[10];
	uint64_t result_time[10];
	for(uint8_t i = 0; i<10; i++)
		delta_time[i] = 0;
	uint8_t iteration = 0;
	uint32_t tx_message;
	
	HAL_UART_Transmit(&huart1, "Init OK", 7, 100);
	while(1)
	{
		*(uint8_t*)&tx_message = iteration%10;
		UWB_SendMessage(&tx_message, sizeof(uint8_t), 1);
		printf("%u\n", tx_message);
		UWB_status rx_status = UWB_WaitForMessage(&responce_message, sizeof(RepeaterMessage));
		if(rx_status==UWB_ERROR)
		{
			printf("UWB_error\n");
			continue;
		}
		if(rx_status==UWB_TIMEOUT)
		{
			printf("UWB_timeout\n");
			continue;
		}
		dwt_readtxtimestamp((uint8_t*)(tx_time+iteration%10));
		dwt_readrxtimestamp((uint8_t*)(&rx_time));
		delta_time[responce_message.sign] = 
			(rx_time >= tx_time[responce_message.sign])?
			(rx_time - tx_time[responce_message.sign]):(rx_time + (1ll<<40) - tx_time[responce_message.sign]);
		if(delta_time[responce_message.correction_sign]<=responce_message.time_correction)
			printf("Error\n");
		printf("%u %llu %llu %llu\n",
			responce_message.correction_sign, delta_time[responce_message.correction_sign],
			responce_message.time_correction,
			delta_time[responce_message.correction_sign]-responce_message.time_correction);
		iteration++;
		HAL_Delay(100);
	}
}

int start_simple_transmiting()
{
	UWB_Init(60000);
	uint8_t tx_message[62];
	while(1)
	{
		for(uint8_t i = 0; i<62; i++)
			tx_message[i] = i;
		UWB_SendMessage(&tx_message, 62, 0);
		//HAL_Delay(500);
	}
}

int start_simple_receiving()
{
	UWB_Init(60000);
	uint8_t rx_message[62];
	while(1)
	{
		UWB_ActivateRX();
		switch(UWB_WaitForMessage(rx_message, 62))
		{
			case UWB_OK:
				for(uint8_t i = 0; i<62; i++)
					if(rx_message[i]!=i)
						printf("%i %i\n", rx_message[i], i);
				printf("Ok\n");
				break;
			case UWB_ERROR:
				printf("Error\n");
				HAL_Delay(1000);
				break;
			case UWB_TIMEOUT:
				printf("Timeout\n");
				break;
		}
	}
}


/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW1000 unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW1000 during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW1000 User Manual.
 * 2. TX to RX delay can be set to 0 to activate reception immediately after transmission. But, on the responder side, it takes time to process the
 *    received frame and generate the response (this has been measured experimentally to be around 70 µs). Using an RX to TX delay slightly less than
 *    this minimum turn-around time allows the application to make the communication efficient while reducing power consumption by adjusting the time
 *    spent with the receiver activated.
 * 3. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive a complete frame sent by the "RX then send a response"
 *    example at the 110k data rate used (around 3 ms).
 * 4. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended frame
 *    length (up to 1023 bytes long) mode which is not used in this example.
 * 5. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 6. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 7. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW1000 User Manual for more details on "interrupts".
 * 9. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/
