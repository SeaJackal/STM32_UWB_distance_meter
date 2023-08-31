#include "crc.h"

#include "EventRecorder.h"

uint16_t calculateCrc16(const void* message_buffer, uint16_t message_length)
{
	uint16_t wcrc = 0xFFFF;
	uint8_t temp;
	uint8_t j;
	for(uint16_t i = 0; i < message_length; i++)
	{
		temp = ((uint8_t*)message_buffer)[i];
		wcrc ^= temp;
		for(j = 0; j<8; j++)
		{
			if(wcrc & 0x0001)
			{
				wcrc >>= 1;
				wcrc ^= 0xA001;
			}
			else
				wcrc >>= 1;
		}
	}
	//EventRecord2(0, wcrc, *(((uint16_t*)message_buffer)+1));
	return wcrc;
}

uint16_t addCrc16(void* message_buffer, uint16_t message_length)
{
	return *(uint16_t*)((uint8_t*)message_buffer+message_length) = calculateCrc16(message_buffer, message_length);
}
uint8_t checkCrc16(const void* message_buffer, uint16_t message_length)
{
	return *(uint16_t*)((uint8_t*)message_buffer+message_length) == calculateCrc16(message_buffer, message_length);
}