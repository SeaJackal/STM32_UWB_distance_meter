#include "uwb_distance_meter.h"

#include <stdlib.h>

#define UWB_DM_BYTE_STRUCT_FIELD(structure, type, offset) *(type*)(structure+offset)
#define UWB_DM_FILL_ID_BYTE(master_bit, agent_id, mess_id) master_bit<<(8-1)|agent_id<<_UWB_DM_AGENT_OFFSET|mess_id
#define UWB_DM_GET_MASTER_BIT(id_byte) id_byte&0b10000000
#define UWB_DM_GET_AGENT_ID(id_byte) ((id_byte&0b01111000)>>3)
#define UWB_DM_GET_MESSAGE_ID(id_byte) (id_byte&0b00000111)

#define UWB_DM_MESSAGE_LENGTH (UWB_DM_AGENT_NUMBER-1)*6+1

void UWB_DM_formMessage(uint8_t* message, UWB_DM_Agent* agent);
UWB_DM_status UWB_DM_parseMessage(uint8_t* message, UWB_DM_Agent* agent, uint8_t* agent_id);

UWB_DM_status UWB_DM_initAgent(UWB_DM_Agent* new_agent, uint8_t id)
{
	if(UWB_Init()==UWB_ERROR)
		return UWB_DM_ERROR;
	new_agent->id = id;
	new_agent->master_id = 0;
	new_agent->is_waiting_responce = 0;
	new_agent->message_id = 0;
	//new_agent->tx_time = malloc(sizeof(uint64_t)*UWB_DM_MESSAGE_NUMBER);
	new_agent->correction_id = malloc(sizeof(uint64_t)*UWB_DM_AGENT_NUMBER);
	new_agent->correction_time = malloc(sizeof(uint64_t)*UWB_DM_AGENT_NUMBER);
	new_agent->delta_time = malloc(sizeof(uint64_t)*UWB_DM_AGENT_NUMBER);
	new_agent->corrected_time = malloc(sizeof(uint64_t)*UWB_DM_AGENT_NUMBER);
	return UWB_DM_OK;
}

UWB_DM_status UWB_DM_startMasterSession(UWB_DM_Agent* agent)
{
	agent->master_id = agent->id;
	uint8_t* message = malloc(UWB_DM_MESSAGE_LENGTH);
	UWB_DM_formMessage(message, agent);
	UWB_SendMessage(message, UWB_DM_MESSAGE_LENGTH, 1);
	//agent->tx_time[agent->message_id] = UWB_GetTxTimestamp64();
	uint64_t tx_time = UWB_GetTxTimestamp64();
	uint8_t is_master;
	uint8_t agent_id;
	uint8_t message_id;
	while(1)
	{
		UWB_WaitForMessage(message, UWB_DM_MESSAGE_LENGTH);
		UWB_ActivateRX();
		uint64_t rx_time = UWB_GetRxTimestamp64();
		if(UWB_DM_parseMessage(message, agent, &agent_id) == UWB_DM_ERROR)
			return UWB_DM_ERROR;
		agent->delta_time[agent_id] = tx_time - rx_time;
	}
}

UWB_DM_status UWB_DM_startSlaveSession(UWB_DM_Agent* agent)
{
	
	UWB_ActivateRX();
}
UWB_DM_status UWB_DM_waitForMessage(UWB_DM_Agent* agent);

void UWB_DM_formMessage(uint8_t* message, UWB_DM_Agent* agent)
{
	UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 0) = 
		UWB_DM_FILL_ID_BYTE((agent->master_id == agent->id), agent->id, agent->message_id);
	for(uint16_t i = agent->id; i!=agent->master_id; i=(i+1)%(UWB_DM_AGENT_NUMBER-1))
	{
		UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 1+6*i) = agent->correction_id[i];
		UWB_DM_BYTE_STRUCT_FIELD(message, uint64_t, 2+6*i) = agent->correction_time[i];
	}
	for(uint16_t i = agent->master_id; i<agent->id; i=(i+1)%(UWB_DM_AGENT_NUMBER-1))
	{
		UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 1+6*i) = UWB_DM_MESSAGE_NUMBER;
		UWB_DM_BYTE_STRUCT_FIELD(message, uint64_t, 2+6*i) = agent->corrected_time[i];
	}
}

UWB_DM_status UWB_DM_parseMessage(uint8_t* message, UWB_DM_Agent* agent, uint8_t* agent_id)
{
	uint8_t id_byte = UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 0);
	*agent_id = UWB_DM_GET_AGENT_ID(id_byte);
	uint8_t message_id = UWB_DM_GET_MESSAGE_ID(id_byte);
	if(UWB_DM_GET_MASTER_BIT(id_byte))
	{
		agent->master_id = *agent_id;
		agent->message_id = message_id;
	}
	else if(message_id!=agent->message_id)
	{
		return UWB_DM_ERROR;
	}
	uint8_t correction_id;
	for(uint16_t i = *agent_id; i!=agent->master_id; i=(i+1)%(UWB_DM_AGENT_NUMBER-1))
	{
		if(UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 1+6*i) != agent->message_id-1)
		{
			agent->corrected_time[*agent_id] = UWB_ERR_TIME;
			continue;
		}
		agent->corrected_time[*agent_id] =  
			agent->delta_time[*agent_id]-UWB_DM_BYTE_STRUCT_FIELD(message, uint64_t, 2+6*i);
	}
	for(uint16_t i = agent->master_id; i<*agent_id; i=(i+1)%(UWB_DM_AGENT_NUMBER-1))
	{
		if(UWB_DM_BYTE_STRUCT_FIELD(message, uint8_t, 1+6*i) != UWB_DM_MESSAGE_NUMBER)
			return UWB_DM_ERROR;
		agent->corrected_time[*agent_id] = UWB_DM_BYTE_STRUCT_FIELD(message, uint64_t, 2+6*i);
	}
	return UWB_DM_OK;
}