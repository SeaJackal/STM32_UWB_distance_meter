#include "uwb_distance_meter.h"

#include <stdlib.h>

#include "uwb_ui.h"

#define UWB_DM_COUNT_DIFFERENCE(later_time, earlier_time) \
		((later_time>earlier_time)?(later_time-earlier_time):(later_time+(1ll<<40)-earlier_time))
#define UWB_DM_COUNT_TIME(delta_1, delta_2, corr_1, corr_2) \
		(delta_1*delta_2 - corr_1*corr_2)/(delta_1 + delta_2 + corr_1 + corr_2)
#define UWB_DM_SET_BIT(reg, bit) reg |= 1<<bit
#define UWB_DM_RESET_BIT(reg, bit) reg &= ~(1<<bit)

void write64to40bit(uint8_t* dest, uint64_t src);
uint64_t read40to64bit(uint8_t* src);

UWB_DM_Status UWB_DM_parseMessage(UWB_DM_Agent* agent);
UWB_DM_Status UWB_DM_fillMessage(UWB_DM_Agent* agent);

UWB_DM_Status UWB_DM_init(UWB_DM_Agent* new_agent, uint8_t agents_number, uint8_t index)
{
	uint8_t message_length = 3+agents_number*15;
	uint64_t* rx_times = malloc(sizeof(uint64_t) * agents_number);
	uint64_t* deltas = malloc(sizeof(uint64_t)*agents_number);
	uint64_t* corrections = malloc(sizeof(uint64_t)*agents_number);
	uint32_t* self_times = malloc(sizeof(uint32_t)*agents_number);
	uint32_t** received_times = malloc(sizeof(uint32_t*)*agents_number);
	for(uint8_t i = 0; i<agents_number; i++)
	{
		rx_times[i] = 0ll;
		deltas[i] = 0ll;
		corrections[i] = 0ll;
		self_times[i] = 0ll;
		received_times[i] = malloc(sizeof(uint32_t)*agents_number);
		for(uint8_t j = 0; j<agents_number; j++)
			received_times[i][j] = 0ll;
	}
	uint8_t* message_buffer = malloc(sizeof(uint8_t)*message_length);
	for(uint8_t i = 0; i<message_length; i++)
		message_buffer[i] = 0;
	UWB_DM_Agent temp = {
		.self_index = index,
		.agents_number = agents_number,
		.message_length = message_length,
		.iteration = 0,
		.speaker = index,
		.state = UWB_DM_SEARCHING_CONNECTION,
		.error_counter = 0,
		.calibration_flag = 0,
		.calibration_not_done_flag = 1,
		.last_calibration_iteration = 0,
		.message_buffer = message_buffer,
		.connection_bits = 0,
		.rx_times = rx_times,
		.tx_time = 0,
		.deltas = deltas,
		.corrections = corrections,
		.self_times = self_times,
		.received_times = received_times
	};
	*new_agent = temp;
	if(UWB_Init(60000)!=UWB_OK)
		return UWB_DM_DW_ERROR;
	if(UWB_ActivateRX()!=UWB_OK)
		return UWB_DM_DW_ERROR;
	return UWB_DM_OK;
}
void UWB_DM_clear(UWB_DM_Agent* agent)
{
	free(agent->rx_times);
	free(agent->deltas);
	free(agent->corrections);
	free(agent->self_times);
	for(uint8_t i = 0; i<agent->agents_number; i++)
		free(agent->received_times[i]);
	free(agent->received_times);
}
UWB_DM_Status UWB_DM_iterate(UWB_DM_Agent* agent)
{
	uint8_t timeouts_counter = 0;
	uint8_t no_connection_flag = 1;
	switch(agent->state)
	{
		case UWB_DM_SEARCHING_CONNECTION:
			while(no_connection_flag)
			{
				switch(UWB_DM_parseMessage(agent))
				{
					case UWB_DM_OK:
						no_connection_flag = 0;
						break;
					case UWB_DM_DW_ERROR:
						return UWB_DM_DW_ERROR;
					case UWB_DM_CONNECTION_ERROR:
						timeouts_counter++;
						if(timeouts_counter>agent->agents_number)
						{
							agent->state = UWB_DM_READY_TO_SEND;
							return UWB_DM_OK;
						}
						if(UWB_ActivateRX()!=UWB_OK)
							return UWB_DM_DW_ERROR;
						break;
				}
			}
			break;
		case UWB_DM_WAITING_MESSAGE:
			if(UWB_DM_parseMessage(agent)==UWB_DM_DW_ERROR)
				return UWB_DM_DW_ERROR;
			if(agent->error_counter >= 2)
			{
				agent->error_counter = 0;
				return UWB_DM_CONNECTION_ERROR;
			}
			break;
		case UWB_DM_READY_TO_SEND:
			if(UWB_DM_fillMessage(agent)==UWB_DM_DW_ERROR)
				return UWB_DM_DW_ERROR;
			break;
	}
	agent->iteration++;
	agent->speaker=(agent->speaker+1)%agent->agents_number;
	if(agent->speaker==agent->self_index)
	{
		//EventRecord2(4, 0, 0);
		agent->state = UWB_DM_READY_TO_SEND;
	}
	else
	{
		agent->state = UWB_DM_WAITING_MESSAGE;
		//EventRecord2(3, 0, 0);
		if(UWB_ActivateRX()!=UWB_OK)
				return UWB_DM_DW_ERROR;
	}
	return UWB_DM_OK;
}

UWB_DM_Status UWB_DM_reset(UWB_DM_Agent* agent)
{
	if(UWB_Init(60000)!=UWB_OK)
		return UWB_DM_DW_ERROR;
	agent->iteration = 0;
	agent->speaker = 0;
	agent->state = UWB_DM_SEARCHING_CONNECTION;
	agent->error_counter = 0;
	agent->connection_bits = 0;
	for(uint8_t i = 0; i<agent->agents_number; i++)
	{
		agent->rx_times[i] = 0ll;
		agent->deltas[i] = 0ll;
		agent->corrections[i] = 0ll;
		agent->self_times[i] = 0ll;
		for(uint8_t j = 0; j<agent->agents_number; j++)
			agent->received_times[i][j] = 0ll;
	}
	if(UWB_ActivateRX()!=UWB_OK)
		return UWB_DM_DW_ERROR;
	return UWB_DM_OK;
}

UWB_DM_Status UWB_DM_parseMessage(UWB_DM_Agent* agent)
{
	switch(UWB_WaitForMessage(agent->message_buffer, agent->message_length))
	{
		case UWB_ERROR:
			return UWB_DM_DW_ERROR;
		case UWB_TIMEOUT:
			UWB_DM_RESET_BIT(agent->connection_bits, agent->speaker);
			return UWB_DM_CONNECTION_ERROR;
		case UWB_OK:
			break;
		default:
			return UWB_DM_DW_ERROR;
	}
	if(agent->message_buffer[0]!=agent->iteration || agent->message_buffer[1]!=agent->speaker)
	{
		if(agent->state != UWB_DM_SEARCHING_CONNECTION)
		{
			agent->error_counter++;
			return UWB_DM_CONNECTION_ERROR;
		}
		agent->iteration = agent->message_buffer[0];
		agent->speaker = agent->message_buffer[1];
	}
	if(agent->message_buffer[2]!=agent->last_calibration_iteration)
	{
		if(agent->calibration_flag)
			agent->calibration_not_done_flag = 1;
		else
		{
			UWB_calibrate(((uint16_t*)(agent->message_buffer+3))[agent->self_index]);
			agent->last_calibration_iteration = agent->iteration;
		}
	}
	uint64_t rx_time = UWB_GetRxTimestamp64();
	if(rx_time == UWB_ERR_TIME)
		return UWB_DM_DW_ERROR;
	uint8_t sender = agent->message_buffer[1];
	agent->corrections[sender] = UWB_DM_COUNT_DIFFERENCE(agent->tx_time, agent->rx_times[sender]);
	agent->rx_times[sender] = rx_time;
	uint64_t delta = read40to64bit(agent->message_buffer+3+agent->self_index*5);
	uint64_t correction = read40to64bit(agent->message_buffer+3+agent->agents_number*5+agent->self_index*5);
	for(uint8_t i = 0; i<agent->agents_number; i++)
		agent->received_times[sender][i] = read40to64bit(agent->message_buffer+3+agent->agents_number*10+i*5);
	agent->self_times[sender] = UWB_DM_COUNT_TIME(
		agent->deltas[sender], delta, agent->corrections[sender], correction);
	agent->deltas[sender] = UWB_DM_COUNT_DIFFERENCE(agent->rx_times[sender], agent->tx_time);
	UWB_DM_SET_BIT(agent->connection_bits, agent->speaker);
	return UWB_DM_OK;
}

UWB_DM_Status UWB_DM_fillMessage(UWB_DM_Agent* agent)
{
	agent->message_buffer[0] = agent->iteration;
	agent->message_buffer[1] = agent->self_index;
	if(agent->calibration_flag && agent->calibration_not_done_flag)
	{
		agent->last_calibration_iteration = agent->iteration;
		agent->calibration_not_done_flag = 0;
		agent->message_buffer[2] = agent->iteration;
	}
	else
	{
		agent->calibration_flag = 0;
		agent->calibration_not_done_flag = 1;
		agent->message_buffer[2] = 0;
		for(uint8_t i = 0; i<agent->agents_number; i++)
		{
			write64to40bit(agent->message_buffer+3+i*5, agent->deltas[i]);
			write64to40bit(agent->message_buffer+3+agent->agents_number*5+i*5, agent->corrections[i]);
			write64to40bit(agent->message_buffer+3+agent->agents_number*10+i*5, agent->self_times[i]);
		}
	}
	if(UWB_SendMessage(agent->message_buffer, agent->message_length, 0)!=UWB_OK)
		return UWB_DM_DW_ERROR;
	agent->tx_time = UWB_GetTxTimestamp64();
	if(agent->tx_time==UWB_ERR_TIME)
		return UWB_DM_DW_ERROR;
	return UWB_DM_OK;
}

uint8_t UWB_DM_countAllTimesNumber(UWB_DM_Agent* agent)
{
	return agent->agents_number*(agent->agents_number-1);
}
uint8_t UWB_DM_countUniqueTimesNumber(UWB_DM_Agent* agent)
{
	return agent->agents_number*(agent->agents_number-1)/2;
}
void UWB_DM_getAllTimes(UWB_DM_Agent* agent, uint32_t* dest)
{
	uint8_t j;
	uint8_t index = 0;
	for(uint8_t i = 0; i<agent->agents_number; i++)
	{
		for(j = 0; j<agent->agents_number; j++)
		{
			if(i != j)
			{
				if(agent->self_index==i)
					dest[index] = agent->self_times[j];
				else
					dest[index] = agent->received_times[i][j];
				index++;
			}
		}
	}
}
void UWB_DM_getUniqueTimes(UWB_DM_Agent* agent, uint32_t* dest)
{
	for(uint8_t i = 0; i<agent->agents_number; i++)
	{
		for(uint8_t j = i+1; j<agent->agents_number; j++)
		{
			if(agent->self_index==i)
				dest[i*(agent->agents_number-1)+j] = agent->self_times[j];
			else
				dest[i*(agent->agents_number-1)+j] = agent->received_times[i][j];
		}
	}
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
void UWB_DM_calibrate(UWB_DM_Agent* agent, uint16_t* delays)
{
	agent->calibration_flag = 1;
	UWB_calibrate(delays[agent->self_index]);
	for(uint8_t i = 0; i<agent->agents_number; i++)
		((uint16_t*)(agent->message_buffer+3))[i] = delays[i];
}