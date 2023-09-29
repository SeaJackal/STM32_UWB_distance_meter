#ifndef UWB_BOAT_INTERFACE_H
#define UWB_BOAT_INTERFACE_H

#include <stdint.h>

typedef enum
{
	UWB_INTERFACE_READY_TO_SEND_OK,
	UWB_INTERFACE_READY_TO_SEND_ERROR,
	UWB_INTERFACE_READY_TO_SEND_TASK_DONE
} UWB_INTERFACE_Status;

void UWB_INTERFACE_markGetMessage();
void UWB_INTERFACE_markSendMessage();
void* UWB_INTERFACE_getTransmitBuffer();
void* UWB_INTERFACE_getReceiveBuffer();
uint8_t UWB_INTERFACE_isLastMessageIncorrect();
uint16_t UWB_INTERFACE_getTransmitLength();
uint16_t UWB_INTERFACE_getReceiveLength();
UWB_INTERFACE_Status UWB_INTERFACE_iterate();

#endif