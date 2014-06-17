#ifndef _ERROR_H_
#define _ERROR_H_ 1

#include "config.h"

/* A simple double-linked-list
 * An empty list consists of a stcut list_head which prev and next point to itself.
 * The first elements(head) prev point to the last element in the list.
 */
struct list_head
{
	struct list_head *next;
	struct list_head *prev;
	void *content;
};

struct mem_error
{
	/* start address of the memory area */
	def_int_t baseaddr;
	/* offset in bytes where the error was detected */
	def_int_t offset;
	/* the expected pattern - this was initial written by the test algorithm */
	def_int_t pattern;
	def_int_t found_pattern;
};

struct list_head* init_list(void);
void add_error(struct list_head *pHead, def_int_t pBase, def_int_t pOff, def_int_t pPat, def_int_t pFound);
void print_errors(struct list_head *pHead);
void free_error_list(struct list_head *pHead);
struct list_head* concat_error_lists(struct list_head *pFirst, struct list_head *pSecond);

#endif
