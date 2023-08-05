#include "uwb_simple_distance_meter.h"

#include <stdlib.h>

#define UWBS_SIGN(message) *(uint8_t*)message
#define UWBS_WRITE_DELTA(message, delta) write64to40bit(message+UWBS_SIGN_SIZE, delta)
#define UWBS_READ_DELTA(message) read40to64bit(message+UWBS_SIGN_SIZE)
#define UWBS_WRITE_CORRECTION(message, correction) write64to40bit(message+UWBS_SIGN_SIZE+UWBS_TIME_SIZE, correction)
#define UWBS_READ_CORRECTION(message) read40to64bit(message+UWBS_SIGN_SIZE+UWBS_TIME_SIZE)
#define UWBS_WRITE_SELF_TIME(message, time) write64to40bit(message+UWBS_SIGN_SIZE+UWBS_TIME_SIZE*2, time)
#define UWBS_READ_SELF_TIME(message) read40to64bit(message+UWBS_SIGN_SIZE+UWBS_TIME_SIZE*2)

#define UWBS_COUNT_DIFFERENCE(later_time, earlier_time) \
		((later_time>earlier_time)?(later_time-earlier_time):(later_time+(1ll<<40)-earlier_time))
#define UWBS_COUNT_TIME(delta_1, delta_2, corr_1, corr_2) \
		(delta_1*delta_2 - corr_1*corr_2)/(delta_1 + delta_2 + corr_1 + corr_2);

void UWBS_fillMessage(UWBS_Agent* agent);
void write64to40bit(uint8_t* dest, uint64_t src);
uint64_t read40to64bit(uint8_t* src);

UWBS_Status UWBS_initAgent(UWBS_Agent* new_agent)
{
	new_agent->iteration = 0;
	new_agent->tx_time = 0;
	new_agent->rx_time = 0;
	new_agent->delta = 0;
	new_agent->correction = 0;
	new_agent->self_time = 0;
	new_agent->received_time = 0;
	if(UWB_Init(UWBS_TIMEOUT)!=UWB_OK)
		return UWBS_DW_ERROR;
	return UWBS_OK;
}
UWBS_Status UWBS_sendMessage(UWBS_Agent* agent)
{
	UWBS_fillMessage(agent);
	if(UWB_SendMessage(agent->message_buffer, UWBS_MESSAGE_SIZE, 1) == UWB_ERROR)
		return UWBS_DW_ERROR;
	agent->tx_time = UWB_GetTxTimestamp64();
	if(agent->tx_time == UWB_ERR_TIME)
		return UWBS_DW_ERROR;
	agent->iteration++;
	agent->correction = UWBS_COUNT_DIFFERENCE(agent->tx_time, agent->rx_time);
	return UWBS_OK;
}
UWBS_Status UWBS_getMessage(UWBS_Agent* agent)
{
	switch(UWB_WaitForMessage(agent->message_buffer, UWBS_MESSAGE_SIZE))
	{
		case UWB_ERROR:
			return UWBS_DW_ERROR;
		case UWB_TIMEOUT:
			return UWBS_RESPONDER_ERROR;
		case UWB_OK:
			break;
	}
	if(agent->iteration!=UWBS_SIGN(agent->message_buffer))
	{
		agent->iteration = UWBS_SIGN(agent->message_buffer)+1;
		return UWBS_RESPONDER_ERROR;
	}
	agent->self_time = UWBS_COUNT_TIME(agent->delta, UWBS_READ_DELTA(agent->message_buffer),
		agent->correction, UWBS_READ_CORRECTION(agent->message_buffer));
	agent->received_time = UWBS_READ_SELF_TIME(agent->message_buffer);
	agent->rx_time = UWB_GetRxTimestamp64();
	if(agent->rx_time == UWB_ERR_TIME)
		return UWBS_DW_ERROR;
	agent->delta = UWBS_COUNT_DIFFERENCE(agent->rx_time, agent->tx_time);
	agent->iteration++;
	return UWBS_OK;
}

void UWBS_fillMessage(UWBS_Agent* agent)
{
	UWBS_SIGN(agent->message_buffer) = agent->iteration;
	UWBS_WRITE_DELTA(agent->message_buffer, agent->delta);
	UWBS_WRITE_CORRECTION(agent->message_buffer, agent->correction);
	UWBS_WRITE_SELF_TIME(agent->message_buffer, agent->self_time);
}

void write64to40bit(uint8_t* dest, uint64_t src)
{
	*(uint32_t*)dest = (uint32_t)src;
	dest[4] = ((uint8_t*)&src)[4];
}
uint64_t read40to64bit(uint8_t* src)
{
	uint64_t temp = *(uint32_t*)src;
	((uint8_t*)&temp)[4] = src[4];
	return temp;
}