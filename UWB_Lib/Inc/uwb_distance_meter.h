#ifndef UWB_DISTANCE_H_
#define UWB_DISTANCE_H_

#include <stdint.h>

#include "uwb_ui.h"

#define _UWB_DM_AGENT_OFFSET 3
#define UWB_DM_AGENT_NUMBER (1<<(sizeof(uint8_t)*8-_UWB_DM_AGENT_OFFSET-1))
#define UWB_DM_MESSAGE_NUMBER (1<<_UWB_DM_AGENT_OFFSET)

typedef enum
{
	UWB_DM_OK,
	UWB_DM_ERROR
} UWB_DM_status;

typedef struct
{
	uint8_t id;
	
	uint8_t is_waiting_responce;
	
	uint8_t message_id;
	uint8_t master_id;
	
	//uint64_t* tx_time;
	uint64_t* delta_time;
	uint64_t* corrected_time;
	
	uint8_t* correction_id;
	uint64_t* correction_time;
} UWB_DM_Agent;

UWB_DM_status UWB_DM_initAgent(UWB_DM_Agent* new_agent, uint8_t id);
UWB_DM_status UWB_DM_startMasterSession(UWB_DM_Agent* agent);
UWB_DM_status UWB_DM_startSlaveSession(UWB_DM_Agent* agent);
UWB_DM_status UWB_DM_waitForMessage(UWB_DM_Agent* agent);

#endif