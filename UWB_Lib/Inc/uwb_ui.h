#ifndef UWB_UI_H_
#define UWB_UI_H_

#include <stdint.h>

#define UWB_ERR_TIME 1ll<<41

typedef enum
{
	UWB_OK,
	UWB_ERROR,
	UWB_TIMEOUT
} UWB_status;

typedef struct
{
	uint8_t sign;
	uint8_t correction_sign;
	uint64_t time_correction;
} RepeaterMessage;

UWB_status UWB_Init(uint32_t rx_timeout);
UWB_status UWB_SendMessage(const void* message, uint16_t length, uint8_t wait_responce_flag);
UWB_status UWB_ActivateRX();
UWB_status UWB_WaitForMessage(void* message, uint32_t length);
uint64_t UWB_GetTxTimestamp64();
uint64_t UWB_GetRxTimestamp64();

#endif