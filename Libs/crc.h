#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

uint16_t calculateCrc16(const void* message_buffer, uint16_t message_length);
uint16_t addCrc16(void* message_buffer, uint16_t message_length);
uint8_t checkCrc16(const void* message_buffer, uint16_t message_length);

#endif
