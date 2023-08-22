#ifndef UWB_DISTANCE_H_
#define UWB_DISTANCE_H_

#include <stdint.h>

typedef enum
{
	UWB_DM_SEARCHING_CONNECTION,
	UWB_DM_WAITING_MESSAGE,
	UWB_DM_READY_TO_SEND
} UWB_DM_State;

typedef enum
{
	UWB_DM_OK,
	UWB_DM_DW_ERROR,
	UWB_DM_CONNECTION_ERROR
}UWB_DM_Status;

typedef struct
{
	uint8_t self_index;
	uint8_t agents_number;
	uint8_t message_length;
	
	uint8_t iteration;
	uint8_t speaker;
	UWB_DM_State state;
	uint8_t error_counter;
	
	uint8_t* message_buffer;
	uint16_t connection_bits;
	
	uint64_t* rx_times;
	uint64_t tx_time;
	
	uint64_t* deltas;
	uint64_t* corrections;
	
	uint64_t* self_times;
	uint64_t* received_times;
} UWB_DM_Agent;

UWB_DM_Status UWB_DM_init(UWB_DM_Agent* new_agent, uint8_t agents_number, uint8_t index);
UWB_DM_Status UWB_DM_reset(UWB_DM_Agent* agent);
UWB_DM_Status UWB_DM_iterate(UWB_DM_Agent* agent);

#endif