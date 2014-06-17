#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "config.h"

/*
 * Creates a new list_head which prev and next pointer point to itself because its an empty list
 */
struct list_head* init_list(void)
{
	struct list_head *ret = NULL;

	ret = (struct list_head*) malloc(sizeof(struct list_head));
	if (ret == NULL)
	{
		perror("malloc");
		return ret;
	}
	ret->prev = ret;
	ret->next = ret;
	ret->content = NULL;

	return ret;
}

/*
 * Insert a new error (struct mem_error) into the list given by pHead.
 * The new element is added to the end of the list.
 */
void add_error(struct list_head *pHead, def_int_t pBase, def_int_t pOff, def_int_t pPat, def_int_t pFound)
{
	struct mem_error *new_one = NULL;
	struct list_head *new_list = NULL;

	new_one = (struct mem_error*)malloc(sizeof(struct mem_error));
	if (new_one == NULL)
	{
		perror("malloc");
		return;
	}

	if (pHead->prev->content != NULL)
	{
		new_list = (struct list_head*)malloc(sizeof(struct list_head));
		if (new_list == NULL)
		{
			perror("malloc");
			free(new_one);
			return;
		}
	}
	else
	{
		new_list = pHead->prev;
	}

	new_list->prev = pHead->prev;
	new_list->next = pHead;
	pHead->prev->next = new_list;
	pHead->prev = new_list;
	if (pHead->next == pHead)
	{
		pHead->next = new_list;
	}
	new_list->content = new_one;

	new_one->baseaddr = pBase;
	new_one->offset = pOff;
	new_one->pattern = pPat;
	new_one->found_pattern = pFound;
}

/*
 * Prints all erros in pHead
 */
void print_errors(struct list_head *pHead)
{
	struct list_head *cur = NULL;
	struct mem_error *error = NULL;

	cur = pHead;
	do
	{
		if (cur->content != NULL)
		{
			error = (struct mem_error*)cur->content;
			#if __WORDSIZE == 64
			printf("Baseaddress:0x%lx\tOffset:0x%lx\tPattern:0x%lx\tFound:0x%lx\n",error->baseaddr,error->offset,error->pattern,error->found_pattern);
			#else
			printf("Baseaddress:0x%x\tOffset:0x%x\tPattern:0x%x\tFound:0x%x\n",error->baseaddr,error->offset,error->pattern,error->found_pattern);
			#endif
		}
		#ifdef DEBUG
		else
		{
			printf("Leer\n");
		}
		#endif
		cur = cur->next;
	}
	while (cur != pHead);
}

/*
 * Free *all* structs in the list given by pHead
 */
void free_error_list(struct list_head *pHead)
{
	struct list_head *cur = NULL, *prev = NULL;

	cur = pHead;
	do
	{
		if (cur->content != NULL)
		{
			free(cur->content);
		}
		prev = cur;
		cur = cur->next;
		free(prev);
	}
	while (cur != pHead);
}

struct list_head* concat_error_lists(struct list_head *pFirst, struct list_head *pSecond)
{
	struct list_head *ret = NULL, *first_prev = NULL, *sec_prev = NULL;

	ret = pFirst;
	first_prev = pFirst->prev;
	sec_prev = pSecond->prev;

	first_prev->next = pSecond;
	pSecond->prev = first_prev;

	sec_prev->next = ret;
	ret->prev = sec_prev;

	return ret;
}
