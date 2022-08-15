// Author(s): Houman Karimi
// Date: 10/06/2021
// Course: COP3404

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

// Standard library includes //

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

// Defines // 

// Structs and enums //

/**
 * @brief ll_node is a node within a linked list. The node will contain a void* to the data,
 * and will also hold a reference to the next node. this is a double ended singly linked list. So we only keep track
 * of the next node, the tail, and the head.
 */
typedef struct ll_node
{
	void* data;
	struct ll_node* next;
}ll_node;

/**
 * @brief linked_list holds the number of elements within the linked list and a reference to the head and tail of the linked list.
 * The linked list will only provide an add(data) operation, freeList(linked_list), and freeListAndValues(linked_list).
 * Traversal of the list can be done manually by looping through the nodes from head to tail.
 */
typedef struct
{
	uint32_t numberOfElements;
	ll_node* head;
	ll_node* tail;
}linked_list;

// Functions //

/**
 * @brief createLinkedList will malloc a linked_list data structure. The function accepts nothing. 
 * The function returns the newly allocated linked list on successful allocation. On error, the function will
 * return NULL. 
 * 
 * @param  void
 * @return new linked_list* on succesful allocation, or NULL if an error occurred 
*/
linked_list* createLinkedList(void);

/**
 * @brief addToList will add the given data to the linked list. The function
 * will accept a linked_list pointer and a void pointer to the data.
 * The function will return a pointer to the given data on successful return and on
 * error it will return NULL.
 * 
 * @param  list   - The linked list where the data will be inserted
 * @param  data   - The data that will be inserted.
 * @return data* on success, or NULL if an error occurred 
*/
void* addToList(linked_list* list, void* data);

/**
 * @brief freeList will free the given linked list and nodes. The function will accept a linked_list pointer
 * to the list that they want freed. The function returns nothing.
 * 
 * @param  list - The list that will be freed.
 * @return void
*/
void freeList(linked_list* list);

/**
 * @brief freeListAndValues will free the given linked list and nodes. The function will also free the data* that it stores
 * within the nodes. The function will accept a linked_list pointer
 * to the list that they want freed. The function returns nothing.
 *
 * @param  list - The list that will be freed.
 * @return void
*/
void freeListAndValues(linked_list* list);

#endif //LINKED_LIST_H

