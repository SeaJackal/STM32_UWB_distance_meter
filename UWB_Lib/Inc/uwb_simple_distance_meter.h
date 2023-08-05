#ifndef UWB_SIMPLE_DISTANCE_H_
#define UWB_SIMPLE_DISTANCE_H_

#include <stdint.h>

#include "uwb_ui.h"

#define UWBS_SIGN_SIZE 1
#define UWBS_TIME_SIZE 5

#define UWBS_TIMEOUT 60000
#define UWBS_ERROR_TIMEOUT 500

#define UWBS_MESSAGE_SIZE UWBS_SIGN_SIZE+UWBS_TIME_SIZE*3

typedef enum
{
	UWBS_OK,
	UWBS_DW_ERROR,
	UWBS_RESPONDER_ERROR
} UWBS_Status;

typedef struct 
{
	uint8_t iteration;
	uint64_t tx_time;
	uint64_t rx_time;
	uint64_t delta;
	uint64_t correction;
	uint64_t self_time;
	uint64_t received_time;
	
	uint8_t message_buffer[UWBS_MESSAGE_SIZE];
} UWBS_Agent;

UWBS_Status UWBS_initAgent(UWBS_Agent* new_agent);
UWBS_Status UWBS_sendMessage(UWBS_Agent* agent);
UWBS_Status UWBS_getMessage(UWBS_Agent* agent);

#endif