#include "linked_list.h"

linked_list* createLinkedList(void)
{
	linked_list* list = (linked_list*)malloc(sizeof(linked_list));
	if (!list)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: An error occurred while mallocing a new linked list.\n");
#endif //_DEBUG
		return NULL;
	}
	memset(list, 0, sizeof(linked_list));

	return list;
}

void* addToList(linked_list* list, void* data)
{
	if (data == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: Null being added to linked list.\n");
#endif //_DEBUG
		return NULL;
	}

	// allocate node
	ll_node* node = (ll_node*)malloc(sizeof(ll_node));
	if (!node)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: Error mallocing a new node within the linked list.\n");
#endif //_DEBUG
		return NULL;
	}
	memset(node, 0, sizeof(ll_node));
	node->data = data;

	// add node to linked list
	if (list->head == NULL)
		list->head = node;
	else 
		list->tail->next = node;

	list->tail = node;
	list->numberOfElements++;
	return data;
}

void freeList(linked_list* list)
{
	if (list == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: given linked_list pointer was NULL, cannot be freed.\n");
#endif //_DEBUG
		return;
	}

	// traverse list and free the nodes
	ll_node* current = list->head;
	ll_node* tmp = NULL;
	while (current != NULL)
	{
		tmp = current;
		current = current->next;
		free(tmp);
	}
	
	free(list);
}

void freeListAndValues(linked_list* list)
{
	if (list == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: given linked_list pointer was NULL, cannot be freed.\n");
#endif //_DEBUG
		return;
	}

	// traverse list and free the nodes and the data within nodes
	ll_node* current = list->head;
	ll_node* tmp = NULL;
	while (current != NULL)
	{
		tmp = current;
		current = current->next;
		free(tmp->data);
		free(tmp);
	}

	free(list);
}
