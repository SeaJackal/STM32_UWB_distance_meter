#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stdint.h>

typedef struct
{
	const uint8_t count;
	uint64_t* data;
	uint8_t* link;
	uint8_t* free_array;
	uint8_t free_count;
	uint8_t head;
	uint8_t tail;
}	List;

typedef struct
{
	List* list;
	uint8_t possition;
	uint8_t prev;
} Iterator_List;

void List_init(List* list, uint8_t count);
uint8_t List_isEmpty(List* list);
void List_addTail(List* list, uint64_t value);

Iterator_List List_getIterator(List* list);
void List_goNext(Iterator_List* iterator);
uint64_t List_getValue(Iterator_List* iterator);
void List_setValue(Iterator_List* iterator, uint64_t value);

void List_add(Iterator_List* iterator, uint64_t value);
void List_remove(Iterator_List* iterator);

#endif