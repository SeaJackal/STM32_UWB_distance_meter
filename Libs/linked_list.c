#include "linked_list.h"

#include <stdlib.h>

void List_init(List* list, uint8_t count)
{
	uint64_t* data = malloc(sizeof(uint64_t)*count);
	uint8_t* link = malloc(sizeof(uint8_t)*count);
	uint8_t* free_array = malloc(sizeof(uint8_t)*count);
	for(uint8_t i = 0; i<count; i++)
		free_array[i] = count-1-i;
	List new_list = {
		.count = count,
		.data = data,
		.link = link,
		.free_array = free_array,
		.free_count = count,
		.head = 0,
		.tail = 0
	};
	*list = new_list;
	for(uint8_t i = 0; i<count; i++)
		list->data[i] = 0ll;
}
uint8_t List_isEmpty(List* list);
Iterator_List List_getIterator(List* list)
{
	Iterator_List iterator = 
	{
		.list = list,
		.possition = list->head,
		.prev = list->tail
	};
	return iterator;
}
void List_goNext(Iterator_List* iterator)
{
	iterator->prev = iterator->possition;
	iterator->possition = iterator->list->link[iterator->possition];
}
uint64_t List_getValue(Iterator_List* iterator)
{
	return iterator->list->data[iterator->possition];
}
void List_setValue(Iterator_List* iterator, uint64_t value)
{
	iterator->list->data[iterator->possition] = value;
}
void List_add(Iterator_List* iterator, uint64_t value)
{
	List* list = iterator->list;
	list->free_count--;
	uint8_t index = list->free_array[list->free_count];
	list->link[index] = list->link[iterator->possition];
	list->link[iterator->possition] = index;
	if(iterator->possition == list->tail)
		list->tail = index;
}
void List_remove(Iterator_List* iterator)
{
	List* list = iterator->list;
	uint8_t index = iterator->possition;
	iterator->possition = list->link[index];
	if(index == list->head)
		list->head = iterator->possition;
	list->free_array[list->free_count] = index;
	list->free_count++;
	list->link[iterator->prev] = iterator->possition;
}
void List_addTail(List* list, uint64_t value)
{
	list->free_count--;
	uint8_t index = list->free_array[list->free_count];
	list->link[list->tail] = index;
	list->tail = index;
	list->data[index] = value;
	list->link[index] = list->head;
}