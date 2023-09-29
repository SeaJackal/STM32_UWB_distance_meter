#include "uwb_boat_interface.h"

#include "uwb_distance_meter.h"

#include <stdlib.h>

#include "crc.h"

#define UWB_INTERFACE_GET_RECEIVE_BIT 		uwb_boat_interface.flags&0x01
#define UWB_INTERFACE_SET_RECEIVE_BIT 		uwb_boat_interface.flags|=0x01
#define UWB_INTERFACE_RESET_RECEIVE_BIT 	uwb_boat_interface.flags&=~0x01
#define UWB_INTERFACE_GET_INCORRECT_MESS_BIT 		uwb_boat_interface.flags&0x02
#define UWB_INTERFACE_SET_INCORRECT_MESS_BIT 		uwb_boat_interface.flags|=0x02
#define UWB_INTERFACE_RESET_INCORRECT_MESS_BIT 	uwb_boat_interface.flags&=~0x02
#define UWB_INTERFACE_GET_INIT_BIT 		uwb_boat_interface.flags&0x04
#define UWB_INTERFACE_SET_INIT_BIT 		uwb_boat_interface.flags|=0x04
#define UWB_INTERFACE_RESET_INIT_BIT 	uwb_boat_interface.flags&=~0x04
#define UWB_INTERFACE_GET_ERROR_BIT 		uwb_boat_interface.flags&0x08
#define UWB_INTERFACE_SET_ERROR_BIT 		uwb_boat_interface.flags|=0x08
#define UWB_INTERFACE_RESET_ERROR_BIT 	uwb_boat_interface.flags&=~0x08
#define UWB_INTERFACE_GET_TASK_DONE_BIT 		uwb_boat_interface.flags&0x10
#define UWB_INTERFACE_SET_TASK_DONE_BIT 		uwb_boat_interface.flags|=0x10
#define UWB_INTERFACE_RESET_TASK_DONE_BIT 	uwb_boat_interface.flags&=~0x10

#define UWB_INTERFACE_INIT_MESS_LENGTH 4

#define UWB_INTERFACE_RECEIVE_TYPE uwb_boat_interface.receive_buffer[0]
#define UWB_INTERFACE_RECEIVE_LENGTH uwb_boat_interface.receive_buffer[1]
#define UWB_INTERFACE_RECEIVE_NUMBER uwb_boat_interface.receive_buffer[2]
#define UWB_INTERFACE_RECEIVE_INDEX uwb_boat_interface.receive_buffer[3]
#define UWB_INTERFACE_RECEIVE_CALIBRATE_DATA_PTR (uint16_t*)(uwb_boat_interface.transmit_buffer+4)

#define UWB_INTERFACE_TRANSMIT_AGENTS_NUMBER uwb_boat_interface.transmit_buffer[0]
#define UWB_INTERFACE_TRANSMIT_SELF_INDEX uwb_boat_interface.transmit_buffer[1]
#define UWB_INTERFACE_TRANSMIT_RESPONCE_CODE uwb_boat_interface.transmit_buffer[2]
#define UWB_INTERFACE_TRANSMIT_ITERATION uwb_boat_interface.transmit_buffer[3]
#define UWB_INTERFACE_TRANSMIT_MESS_LENGTH *(uint16_t*)(uwb_boat_interface.transmit_buffer+4)
#define UWB_INTERFACE_TRANSMIT_CONN_FIELD *(uint16_t*)(uwb_boat_interface.transmit_buffer+6)
#define UWB_INTERFACE_TRANSMIT_DATA_PTR (uint32_t*)(uwb_boat_interface.transmit_buffer+8)

#define UWB_INTERFACE_ADD_CRC addCrc16(uwb_boat_interface.transmit_buffer, uwb_boat_interface.transmit_length)
#define UWB_INTERFACE_CHECK_CRC checkCrc16(uwb_boat_interface.receive_buffer, uwb_boat_interface.receive_length)

#define UWB_INTERFACE_RESPONCE_OK 	0x00
#define UWB_INTERFACE_RESPONCE_ERROR_DW 		0x01
#define UWB_INTERFACE_RESPONCE_ERROR_CONN	0x02
#define UWB_INTERFACE_RESPONCE_INIT_DONE	0x03
#define UWB_INTERFACE_RESPONCE_CALIBRATION_DONE	0x04

#define UWB_INTERFACE_DEFAULT_NUMBER 4
#define UWB_INTERFACE_DEFAULT_INDEX 1

uint8_t init_buffer[UWB_INTERFACE_INIT_MESS_LENGTH+2] = {0, 0, UWB_INTERFACE_DEFAULT_NUMBER, UWB_INTERFACE_DEFAULT_INDEX, 0, 0};

typedef enum
{
	UWB_INTERFACE_WAITING_INIT,
	UWB_INTERFACE_WAITING_CALIBRATION,
	UWB_INTERFACE_SENDING_DATA,
	UWB_INTERFACE_RESET_REQUIRED
} UWB_INTERFACE_State;

typedef enum
{
	UWB_INTERFACE_MESSAGE_INIT = 0,
	UWB_INTERFACE_MESSAGE_CALIBRATE = 1,
	UWB_INTERFACE_MESSAGE_RESET = 2
} UWB_INTERFACE_MessageType;

struct
{
	UWB_INTERFACE_State state;
	
	UWB_DM_Agent agent;

	uint8_t* transmit_buffer;
	uint8_t* receive_buffer;
	
	uint16_t transmit_length;
	uint16_t receive_length;
	
	uint8_t flags;
	
	uint8_t iteration;
	
} uwb_boat_interface = {
	.state = UWB_INTERFACE_WAITING_INIT,
	.flags = 0x00,
	.iteration = 0,
	.receive_buffer = init_buffer,
	.receive_length = UWB_INTERFACE_INIT_MESS_LENGTH};

void UWB_INTERFACE_initMessageBuffer();
void UWB_INTERFACE_initSelf();
void UWB_INTERFACE_calibrate();
uint8_t UWB_INTERFACE_repairSelf();

void UWB_INTERFACE_parseMessage();
void UWB_INTERFACE_manageDM();
void UWB_INTERFACE_iterateDM();
uint8_t UWB_INTERFACE_formMessage();

void UWB_INTERFACE_markGetMessage()
{
	UWB_INTERFACE_SET_RECEIVE_BIT;
}
void UWB_INTERFACE_markSendMessage()
{
	UWB_INTERFACE_RESET_RECEIVE_BIT;
}
void* UWB_INTERFACE_getTransmitBuffer()
{
	return uwb_boat_interface.transmit_buffer;
}
void* UWB_INTERFACE_getReceiveBuffer()
{
	return uwb_boat_interface.receive_buffer;
}
uint16_t UWB_INTERFACE_getTransmitLength()
{
	return uwb_boat_interface.transmit_length + 2;
}
uint16_t UWB_INTERFACE_getReceiveLength()
{
	return uwb_boat_interface.receive_length + 2;
}
UWB_INTERFACE_Status UWB_INTERFACE_iterate()
{
	if(UWB_INTERFACE_GET_RECEIVE_BIT)
		UWB_INTERFACE_parseMessage();
	UWB_INTERFACE_manageDM();
	UWB_INTERFACE_ADD_CRC;
	if(UWB_INTERFACE_GET_ERROR_BIT)
	{
		UWB_INTERFACE_RESET_ERROR_BIT;
		return UWB_INTERFACE_READY_TO_SEND_ERROR;
	}
	else if(UWB_INTERFACE_GET_TASK_DONE_BIT)
	{
		UWB_INTERFACE_RESET_TASK_DONE_BIT;
		return UWB_INTERFACE_READY_TO_SEND_TASK_DONE;
	}
	else
		return UWB_INTERFACE_READY_TO_SEND_OK;
}
uint8_t UWB_INTERFACE_isLastMessageIncorrect()
{
	return UWB_INTERFACE_GET_INCORRECT_MESS_BIT;
}
void UWB_INTERFACE_initMessageBuffer()
{
	if(UWB_INTERFACE_GET_INIT_BIT)
	{
		free(uwb_boat_interface.transmit_buffer);
		free(uwb_boat_interface.receive_buffer);
	}
	else
		UWB_INTERFACE_SET_INIT_BIT;
	uwb_boat_interface.transmit_length = 
		8+sizeof(uint32_t)*UWB_INTERFACE_RECEIVE_NUMBER*(UWB_INTERFACE_RECEIVE_NUMBER-1);
	//uwb_boat_interface.receive_length =
	//	2+sizeof(uint16_t)*UWB_INTERFACE_RECEIVE_NUMBER;
	uwb_boat_interface.receive_length = UWB_INTERFACE_INIT_MESS_LENGTH;
	uwb_boat_interface.transmit_buffer = malloc(sizeof(uint8_t)*(uwb_boat_interface.transmit_length+2));
	UWB_INTERFACE_TRANSMIT_AGENTS_NUMBER = UWB_INTERFACE_RECEIVE_NUMBER;
	UWB_INTERFACE_TRANSMIT_SELF_INDEX = UWB_INTERFACE_RECEIVE_INDEX;
	UWB_INTERFACE_TRANSMIT_MESS_LENGTH = uwb_boat_interface.transmit_length+2;
	UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_OK;
	uwb_boat_interface.receive_buffer = malloc(sizeof(uint8_t)*(uwb_boat_interface.receive_length+2));
}
void UWB_INTERFACE_initSelf()
{
	if(UWB_INTERFACE_GET_INIT_BIT)
		UWB_DM_clear(&uwb_boat_interface.agent);
	if(UWB_DM_init(&uwb_boat_interface.agent,
		UWB_INTERFACE_RECEIVE_NUMBER, UWB_INTERFACE_RECEIVE_INDEX)==UWB_DM_DW_ERROR)
	{
		uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
		UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_ERROR_DW;
		UWB_INTERFACE_SET_ERROR_BIT;
	}
	else
	{
		uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
		UWB_INTERFACE_SET_TASK_DONE_BIT;
	}
	UWB_INTERFACE_initMessageBuffer();
}
uint8_t UWB_INTERFACE_repairSelf()
{
	if(UWB_DM_reset(&uwb_boat_interface.agent)==UWB_DM_DW_ERROR)
	{
		uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
		return 1;
	}
	else
	{
		uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
		return 0;
	}
}

void UWB_INTERFACE_parseMessage()
{
	UWB_INTERFACE_RESET_RECEIVE_BIT;
	if(!UWB_INTERFACE_CHECK_CRC)
		UWB_INTERFACE_SET_INCORRECT_MESS_BIT;
	else
	{
		UWB_INTERFACE_RESET_INCORRECT_MESS_BIT;
		switch((UWB_INTERFACE_MessageType)UWB_INTERFACE_RECEIVE_TYPE)
		{
			case UWB_INTERFACE_MESSAGE_INIT:
				uwb_boat_interface.state = UWB_INTERFACE_WAITING_INIT;
				break;
				//return UWB_INTERFACE_initSelf();
			case UWB_INTERFACE_MESSAGE_CALIBRATE:
				uwb_boat_interface.state = UWB_INTERFACE_WAITING_CALIBRATION;
				//if(UWB_DM_calibrate(&uwb_boat_interface.agent)==UWB_DM_DW_ERROR)
				break;
			case UWB_INTERFACE_MESSAGE_RESET:
				uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
				//UWB_INTERFACE_repairSelf();
				break;
		}
	}
}
void UWB_INTERFACE_manageDM()
{
	switch(uwb_boat_interface.state)
	{
		case UWB_INTERFACE_WAITING_INIT:
			UWB_INTERFACE_initSelf();
			break;
		case UWB_INTERFACE_WAITING_CALIBRATION:
			UWB_INTERFACE_calibrate();
			break;
		case UWB_INTERFACE_SENDING_DATA:
			UWB_INTERFACE_iterateDM();
			break;
		case UWB_INTERFACE_RESET_REQUIRED:
			if(UWB_INTERFACE_repairSelf())
				UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_ERROR_DW;
			else
				UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_OK;
			break;
	}
}
void UWB_INTERFACE_iterateDM()
{
	switch(UWB_DM_iterate(&uwb_boat_interface.agent))
	{
		case UWB_DM_DW_ERROR:
			UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_ERROR_DW;
			UWB_INTERFACE_SET_ERROR_BIT;
			uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
			break;
		case UWB_DM_CONNECTION_ERROR:
			UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_ERROR_CONN;
			UWB_INTERFACE_SET_ERROR_BIT;
			uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
			break;
		case UWB_DM_OK:
			UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_OK;
			UWB_INTERFACE_TRANSMIT_CONN_FIELD = uwb_boat_interface.agent.connection_bits;
			UWB_INTERFACE_TRANSMIT_ITERATION = uwb_boat_interface.iteration;
			UWB_DM_getAllTimes(&uwb_boat_interface.agent, UWB_INTERFACE_TRANSMIT_DATA_PTR);
			uwb_boat_interface.iteration++;
			break;
	}
}

void UWB_INTERFACE_calibrate()
{
	UWB_INTERFACE_TRANSMIT_RESPONCE_CODE = UWB_INTERFACE_RESPONCE_CALIBRATION_DONE;
	UWB_INTERFACE_SET_TASK_DONE_BIT;
}