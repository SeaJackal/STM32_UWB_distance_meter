#include "uwb_boat_interface.h"

#include "uwb_distance_meter.h"

#include <stdlib.h>

#include "crc.h"

#define UWB_INTERFACE_GET_RECEIVE_BIT 		uwb_boat_interface.flags&0x01
#define UWB_INTERFACE_SET_RECEIVE_BIT 		uwb_boat_interface.flags|=0x01
#define UWB_INTERFACE_RESET_RECEIVE_BIT 	uwb_boat_interface.flags&=~0x01
#define UWB_INTERFACE_GET_TRANSMIT_BIT 		uwb_boat_interface.flags&0x02
#define UWB_INTERFACE_SET_TRANSMIT_BIT 		uwb_boat_interface.flags|=0x02
#define UWB_INTERFACE_RESET_TRANSMIT_BIT 	uwb_boat_interface.flags&=~0x02
#define UWB_INTERFACE_GET_INIT_BIT 		uwb_boat_interface.flags&0x04
#define UWB_INTERFACE_SET_INIT_BIT 		uwb_boat_interface.flags|=0x04
#define UWB_INTERFACE_RESET_INIT_BIT 	uwb_boat_interface.flags&=~0x04
#define UWB_INTERFACE_GET_REINIT_BIT 		uwb_boat_interface.flags&0x08
#define UWB_INTERFACE_SET_REINIT_BIT 		uwb_boat_interface.flags|=0x08
#define UWB_INTERFACE_RESET_REINIT_BIT 	uwb_boat_interface.flags&=~0x08

#define UWB_INTERFACE_INIT_MESS_LENGTH 2

#define UWB_INTERFACE_INIT_NUMBER uwb_boat_interface.init_buffer[0]
#define UWB_INTERFACE_INIT_INDEX uwb_boat_interface.init_buffer[1]

#define UWB_INTERFACE_AGENTS_NUMBER uwb_boat_interface.message_buffer[0]
#define UWB_INTERFACE_SELF_INDEX uwb_boat_interface.message_buffer[1]
#define UWB_INTERFACE_ERROR_CODE uwb_boat_interface.message_buffer[2]
#define UWB_INTERFACE_ITERATION uwb_boat_interface.message_buffer[3]
#define UWB_INTERFACE_MESS_LENGTH *(uint16_t*)(uwb_boat_interface.message_buffer+4)
#define UWB_INTERFACE_CONN_FIELD *(uint16_t*)(uwb_boat_interface.message_buffer+6)
#define UWB_INTERFACE_DATA_PTR (uint32_t*)(uwb_boat_interface.message_buffer+8)

#define UWB_INTERFACE_ADD_CRC addCrc16(uwb_boat_interface.message_buffer, uwb_boat_interface.message_length)
#define UWB_INTERFACE_CHECK_CRC checkCrc16(uwb_boat_interface.init_buffer, UWB_INTERFACE_INIT_MESS_LENGTH)

#define UWB_INTERFACE_ERROR_NONE 	0x00
#define UWB_INTERFACE_ERROR_DW 		0x01
#define UWB_INTERFACE_ERROR_CONN	0x02

typedef struct
{
	UWB_INTEFACE_State state;
	
	UWB_DM_Agent agent;

	uint8_t* message_buffer;
	uint8_t init_buffer[UWB_INTERFACE_INIT_MESS_LENGTH+2];
	
	uint16_t message_length;
	
	uint8_t flags;
	
	uint8_t iteration;
	
} UWB_Interface;

UWB_Interface uwb_boat_interface;

uint8_t UWB_INTERFACE_initMessageBuffer();

void UWB_INTERFACE_init()
{
	uwb_boat_interface.state = UWB_INTERFACE_WAITING_INIT;
	uwb_boat_interface.flags = 0x00;
	uwb_boat_interface.iteration = 0;
}
void UWB_INTERFACE_markGetMessage()
{
	UWB_INTERFACE_SET_RECEIVE_BIT;
}
void UWB_INTERFACE_markSendMessage()
{
	UWB_INTERFACE_RESET_RECEIVE_BIT;
}
void* UWB_INTERFACE_getMessageBuffer()
{
	return uwb_boat_interface.message_buffer;
}
void* UWB_INTERFACE_getInitBuffer()
{
	return uwb_boat_interface.init_buffer;
}
uint16_t UWB_INTERFACE_getMessageLength()
{
	return uwb_boat_interface.message_length + 2;
}
uint16_t UWB_INTERFACE_getInitLength()
{
	return UWB_INTERFACE_INIT_MESS_LENGTH +2;
}
UWB_INTERFACE_Status UWB_INTERFACE_iterate()
{
	switch(uwb_boat_interface.state)
	{
		case UWB_INTERFACE_WAITING_INIT:
			if(UWB_INTERFACE_GET_RECEIVE_BIT || UWB_INTERFACE_GET_REINIT_BIT)
			{
				UWB_INTERFACE_RESET_RECEIVE_BIT;
				if(UWB_INTERFACE_initMessageBuffer())
					return UWB_INTERFACE_INCORRECT_MESSAGE;
				if(UWB_DM_init(&uwb_boat_interface.agent,
					UWB_INTERFACE_INIT_NUMBER, UWB_INTERFACE_INIT_INDEX)==UWB_DM_DW_ERROR)
				{
					UWB_INTERFACE_SET_REINIT_BIT;
					UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_DW;
					UWB_INTERFACE_ADD_CRC;
					return UWB_INTERFACE_READY_TO_SEND_ERROR;
				}
				UWB_INTERFACE_RESET_REINIT_BIT;
				uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
				UWB_INTERFACE_ADD_CRC;
				return UWB_INTERFACE_READY_TO_SEND_OK;
			}
			return UWB_INTERFACE_NO_MESSAGE;
		case UWB_INTERFACE_SENDING_DATA:
			switch(UWB_DM_iterate(&uwb_boat_interface.agent))
			{
				case UWB_DM_DW_ERROR:
					UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_DW;
					if(UWB_DM_reset(&uwb_boat_interface.agent)==UWB_DM_DW_ERROR)
						uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
					else
						uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
					UWB_INTERFACE_ADD_CRC;
					return UWB_INTERFACE_READY_TO_SEND_ERROR;
				case UWB_DM_CONNECTION_ERROR:
					UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_CONN;
					if(UWB_DM_reset(&uwb_boat_interface.agent)==UWB_DM_DW_ERROR)
						uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
					else
						uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
					UWB_INTERFACE_ADD_CRC;
					return UWB_INTERFACE_READY_TO_SEND_ERROR;
				case UWB_DM_OK:
					UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_NONE;
					UWB_INTERFACE_CONN_FIELD = uwb_boat_interface.agent.connection_bits;
					UWB_DM_getAllTimes(&uwb_boat_interface.agent, UWB_INTERFACE_DATA_PTR);
					UWB_INTERFACE_ADD_CRC;
					uwb_boat_interface.iteration++;
					return UWB_INTERFACE_READY_TO_SEND_OK;
			}
			break;
		case UWB_INTERFACE_RESET_REQUIRED:
			UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_DW;
			if(UWB_DM_reset(&uwb_boat_interface.agent)==UWB_DM_DW_ERROR)
				uwb_boat_interface.state = UWB_INTERFACE_RESET_REQUIRED;
			else
				uwb_boat_interface.state = UWB_INTERFACE_SENDING_DATA;
			UWB_INTERFACE_ADD_CRC;
			return UWB_INTERFACE_READY_TO_SEND_ERROR;		
	}
}
uint8_t UWB_INTERFACE_initMessageBuffer()
{
	if(!UWB_INTERFACE_CHECK_CRC)
		return 1;
	if(UWB_INTERFACE_GET_INIT_BIT)
		free(uwb_boat_interface.message_buffer);
	uwb_boat_interface.message_length = 
		8+sizeof(uint32_t)*UWB_INTERFACE_INIT_NUMBER*(UWB_INTERFACE_INIT_NUMBER-1);
	uwb_boat_interface.message_buffer = malloc(sizeof(uint8_t)*(uwb_boat_interface.message_length+2));
	UWB_INTERFACE_AGENTS_NUMBER = UWB_INTERFACE_INIT_NUMBER;
	UWB_INTERFACE_SELF_INDEX = UWB_INTERFACE_INIT_INDEX;
	UWB_INTERFACE_MESS_LENGTH = uwb_boat_interface.message_length+2;
	UWB_INTERFACE_ERROR_CODE = UWB_INTERFACE_ERROR_NONE;
	UWB_INTERFACE_SET_INIT_BIT;
	return 0;
}