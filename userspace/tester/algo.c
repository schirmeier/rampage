#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "algo.h"
#include "config.h"

#define MOD_SZ		20

// FIXME: __* ist ein für den Compiler reservierter Namensraum!
static struct list_head* __test_mv_inv(def_int_t *pStart, def_int_t *pEnd,int pIter, def_int_t pPattern);
static struct list_head* __test_inv_32(def_int_t *pStart, def_int_t *pEnd, def_int_t pPattern, def_int_t pHBytes, def_int_t pLBytes, def_int_t pSVal, int pOff);
static struct list_head* __test_mod(def_int_t *pStart, def_int_t *pEnd, def_int_t pPatter, int pOffset, int pIter);
static inline ulong roundup(ulong value, ulong mask)
{
	return (value + mask) & ~mask;
}

struct list_head* test_address_bits(def_int_t *pStart,def_int_t *pEnd,def_int_t pPattern)
{
	struct list_head *errors  = NULL;
	def_int_t pattern = 0, pattern_not = 0, mask = 0;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL;
	int j = 0, i = 0;

	pattern = pPattern;
	pattern_not = ~pattern;
	start = pStart;
	end = pEnd;

	for (j = 0; j < 2; j++)
	{
		if (j == 1)
		{
			pattern = pattern_not;
			pattern_not = ~pattern;
		}
		/* Set pattern in our lowest multiple of 0x20000 */
		start = (def_int_t *)roundup((def_int_t)start, 0x1ffff);
		*start = pattern;

		#ifdef DEBUG
		printf("Pattern:0x%x, ~Pattern:0x%x\n",pattern,pattern_not);
		#endif
		for (i = 0; i < 100; i++)
		{
			for (mask = 4; mask > 0; mask = mask << 1)
			{
				cur = (def_int_t*)((def_int_t)start | mask);
				if (cur == start)
				{
					continue;
				}
				if (cur >= end)
				{
					break;
				}
				#ifdef DEBUG
				printf("Testing:0x%x,mask:%d\n",(def_int_t)cur,mask);
				#endif
				*cur = pattern_not;
				if (*start != pattern)
				{
					if (errors == NULL)
					{
						errors = init_list();
					}
					add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),pattern,*start);
					i = 1000;
				}
			}
		}
	}

	/* memtest86+ würde hier jedes Segment testen und dabei jeweils um bank/4-Adressen voranschreiten  */

	return errors;
}

struct list_head* test_address_own(def_int_t *pStart,def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL;

	start = pStart;
	end = pEnd;

	for (cur = start; cur < end; cur++)
	{
		#ifdef DEBUG
		printf("Testing:0x%x\n",(def_int_t)cur);
		#endif
		*cur = (def_int_t)cur;

		/* we corrupt some memory  */
		/* *cur = *cur << 2;*/
	}

	for (cur = start; cur < end; cur++)
	{
		if (*cur != (def_int_t)cur)
		{
			if (errors == NULL)
			{
				errors = init_list();
			}
			add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),(def_int_t)cur,*cur);
		}
	}

	return errors;
}

/* Wird 15 mal ausgeführt */
struct list_head* test_random_numbers(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL, seed = 0, num = 0;
	int  i = 0;

	start = pStart;
	end = pEnd;
	seed = time(NULL);
	srand(seed);

	for (cur = start; cur < end; cur++)
	{
		#ifdef DEBUG
		printf("Testing:0x%x\n",(def_int_t)cur);
		#endif
		#if __WORDSIZE == 64
                num  = (def_int_t)rand();
                num |= (def_int_t)rand() << 32;
		#else
		num = rand();
		#endif
		*cur = num;
	}

	for (i = 0; i < 2; i++)
	{
		srand(seed);
		for (cur = start; cur < end; cur++)
		{
			#if __WORDSIZE == 64
                        num  = (def_int_t)rand();
                        num |= (def_int_t)rand() << 32;
			#else
			num = rand();
			#endif
			if (i == 1)
			{
				num = ~num;
			}
			if (*cur != num)
			{
				if (errors == NULL)
				{
					errors = init_list();
				}
				add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),num,(def_int_t)*cur);
			}
			*cur = ~num;

			#ifdef CORRUPT
			/* we corrupt some memory  */
			 *cur = *cur << 2;
			#endif
		}
	}

	return errors;
}

struct list_head* test_mv_inv_rand(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	def_int_t seed = 0, pattern = 0;
	int i = 0;

	seed = time(NULL);
	srand(seed);

	/* Laut Ausgabe in memtest86+ werden entweder 25 oder 50 Iterationen ausgeführt */
	for (i = 0; i < 25; i++)
	{
		pattern = rand();
		errors = __test_mv_inv(pStart,pEnd,2,pattern);
		if (errors != NULL)
		{
			return errors;
		}
	}

	return errors;
}

/* similiar to memtest86+ test 5 */
struct list_head* test_block_move(def_int_t *pStart, def_int_t *pEnd)
{
        struct list_head *errors = NULL;
        uint32_t *start, *mid, *end, *ptr;
        uint32_t value, carry, len;
        int iter;

        /* casting to 32 bit to copy the memtest86+ original more closely */
        start = (uint32_t *)pStart;
        end   = (uint32_t *)pEnd;

        /* do not test if the memory block is too small */
        if (end - start < 16)
                return NULL;

        /* initialize memory */
        ptr = start;
        value = 1;
        carry = 0;

        while ((end - ptr) >= 16) {
                *ptr++ =  value;
                *ptr++ =  value;
                *ptr++ =  value;
                *ptr++ =  value;

                *ptr++ = ~value;
                *ptr++ = ~value;
                *ptr++ =  value;
                *ptr++ =  value;

                *ptr++ =  value;
                *ptr++ =  value;
                *ptr++ = ~value;
                *ptr++ = ~value;

                *ptr++ =  value;
                *ptr++ =  value;
                *ptr++ = ~value;
                *ptr++ = ~value;

                uint32_t newcarry = !!(value & 0x8000000U);
                value = (value << 1) | carry;
                carry = newcarry;
        }

        /* do some block moves */
        len = end - start;
        mid = start + len / 2;

        for (iter = 0; iter < 80; iter++) {
                memmove(mid, start, len*4/2);
                memmove(start + 32/4, mid, 4/2*len - 32);
                memmove(start, mid + len/2 - 32/4, 32);
        }

        /* check if adjacent values still match */
        ptr = start;
        while (ptr < end) {
                if (*ptr != *(ptr+1)) {
                        if (errors == NULL)
                                errors = init_list();

                        // FIXME: This is wrong! The entire memory range where block moves were done must be marked bad!
                        add_error(errors, (def_int_t)pStart, 4 * (ptr - start), *ptr, *(ptr+1));
                }

                ptr += 2;
        }

        return errors;
}

struct list_head* test_mv_inv_onezeros(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	def_int_t pattern = 0x0;


	errors = __test_mv_inv(pStart,pEnd,60,pattern);
	if (errors != NULL)
	{
		return errors;
	}
	pattern = ~pattern;
	errors = __test_mv_inv(pStart,pEnd,60,pattern);

	return errors;
}

struct list_head* test_mv_inv_eightpat(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	def_int_t pattern = 0x80, test_pattern = 0;
	int i = 0;

	for (i = 0;i < 8; i++, pattern = pattern >> 1)
	{
		#if __WORDSIZE == 64
		test_pattern = pattern | (pattern << 8) | (pattern << 16) | (pattern << 24) | (pattern << 32) | (pattern << 40) | (pattern << 48) | (pattern << 56);
		#else
		test_pattern = pattern | (pattern << 8) | (pattern << 16) | (pattern << 24);
		#endif
		errors = __test_mv_inv(pStart,pEnd,60,test_pattern);
		if (errors != NULL)
		{
			return errors;
		}
		test_pattern = ~test_pattern;
		errors = __test_mv_inv(pStart,pEnd,60,test_pattern);
		if (errors != NULL)
		{
			return errors;
		}
	}

	return errors;
}

struct list_head* test_mv_inv_32(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	int i = 0;
	def_int_t pattern = 0;

	for (i = 0, pattern = 1; pattern > 0; pattern = pattern << 1, i++)
	{
          errors = __test_inv_32(pStart,pEnd,pattern, 1, 1UL << (sizeof(def_int_t)*8-1), 0, i);
		if (errors != NULL)
		{
			return errors;
		}
		errors = __test_inv_32(pStart,pEnd,~pattern, ~(1UL), ~(1UL << (sizeof(def_int_t)*8-1)), 1, i);
		if (errors != NULL)
		{
			return errors;
		}
	}


	return errors;
}

struct list_head* test_mod_20(def_int_t *pStart, def_int_t *pEnd)
{
	struct list_head *errors  = NULL;
	int j = 0, i = 0, seed = 0;
	def_int_t pattern = 0;

	seed = time(NULL);
	srand(seed);

	for (j = 0; j < 3; j++)
	{
		#if __WORDSIZE == 64
		pattern = ((def_int_t)rand()) << 32 | ((def_int_t)rand());
		#else
		pattern = rand();
		#endif
		for (i = 0; i< MOD_SZ; i++)
		{
			errors = __test_mod(pStart,pEnd,pattern, i, 2);
			if (errors != NULL)
			{
				return errors;
			}
			pattern = ~pattern;
			errors = __test_mod(pStart,pEnd,pattern, i, 2);
			if (errors != NULL)
			{
				return errors;
			}
			pattern = ~pattern;
		}
	}

	return errors;
}

static struct list_head* __test_mod(def_int_t *pStart, def_int_t *pEnd, def_int_t pPattern, int pOffset, int pIter)
{
	struct list_head *errors  = NULL;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL, pattern = 0, pattern_not = 0;
	int  iter = 0,i = 0, k = 0;

	start = pStart;
	end = pEnd;
	pattern = pPattern;
	pattern_not = ~pattern;
	iter = pIter;

	for (cur = start + (def_int_t)pOffset; cur < end; cur += MOD_SZ)
	{
		*cur = pattern;
	}

	for (i = 0; i < iter; i++)
	{
                k = 0;
		for (cur = start; cur < end; cur++)
		{
			if (k != pOffset)
			{
				*cur = pattern_not;
			}
			if (++k > MOD_SZ-1)
			{
				k = 0;
			}
		}
	}

	for (cur = start + (def_int_t)pOffset; cur < end; cur += MOD_SZ)
	{
		if (*cur != pattern)
		{
			if (errors == NULL)
			{
				errors = init_list();
			}
			add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),pattern,(def_int_t)*cur);
		}
	}

	return errors;
}

static struct list_head* __test_inv_32(def_int_t *pStart, def_int_t *pEnd, def_int_t pPattern, def_int_t pLBytes, def_int_t pHBytes, def_int_t pSVal, int pOff)
{
	struct list_head *errors  = NULL;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL, pattern = 0, pattern3 = 0;
	int  iter = 1,i = 0, k = 0;

	start = pStart;
	end = pEnd;
	pattern = pPattern;
	pattern3 = pSVal << (8*sizeof(def_int_t)-1);
	k = pOff;

	for (cur = start; cur < end; cur++)
	{
		#ifdef DEBUG
		printf("Testing:0x%x\n",(def_int_t)cur);
		#endif
		*cur = pattern;
		if (++k >= 8*sizeof(def_int_t))
		{
			pattern = pLBytes;
			k = 0;
		}
		else
		{
			pattern = pattern << 1;
			pattern |= pSVal;
		}

		#ifdef CORRUPT
		/* we corrupt some memory  */
		 *cur = *cur << 2;
		#endif
	}

	for (i = 0; i < iter; i++)
	{
		k = pOff;
		pattern = pPattern;
		for (cur = start; cur < end; cur++)
		{
			if (*cur != pattern)
			{
				if (errors == NULL)
				{
					errors = init_list();
				}
				add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),pattern,(def_int_t)*cur);
				
			}
			*cur = ~pattern;
			if (++k >= sizeof(def_int_t)*8)
			{
				pattern = pLBytes;
				k = 0;
			}
			else
			{
				pattern = pattern << 1;
				pattern |= pSVal;
			}
		}
		pattern = pLBytes;
		if ( 0 != (k = (k-1) & (8*sizeof(def_int_t)-1)) )
		{
			pattern = (pattern << k);
			if ( pSVal )
				pattern |= ((pSVal << k) - 1);
		}
		k++;
		for (cur = end - 1; cur >= start; cur--)
		{
			if (*cur != ~pattern)
			{
				if (errors == NULL)
				{
					errors = init_list();
				}
				add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),~pattern,(def_int_t)*cur);
			}
			*cur = pattern;
			if (--k <= 0)
			{
				pattern = pHBytes;
				k = sizeof(def_int_t)*8;
			}
			else
			{
				pattern = pattern >> 1;
				pattern |= pattern3;
			}
		}
	}

	return errors;
}

static struct list_head* __test_mv_inv(def_int_t *pStart, def_int_t *pEnd,int pIter, def_int_t pPattern)
{
	struct list_head *errors  = NULL;
	volatile def_int_t *cur = NULL;
	def_int_t *start = NULL, *end = NULL, pattern = 0, pattern_not = 0;
	int  iter = 60,i = 0;

	start = pStart;
	end = pEnd;
	pattern = pPattern;
	pattern_not = ~pattern;
	iter = pIter;

	for (cur = start; cur < end; cur++)
	{
		#ifdef DEBUG
		printf("Testing:0x%x\n",(def_int_t)cur);
		#endif
		*cur = pattern;

		#ifdef CORRUPT
		/* we corrupt some memory  */
		 *cur = *cur << 2;
		#endif
	}

	for (i = 0; i < iter; i++)
	{
		for (cur = start; cur < end; cur++)
		{
			if (*cur != pattern)
			{
				if (errors == NULL)
				{
					errors = init_list();
				}
				add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),pattern,(def_int_t)*cur);
			}
			*cur = pattern_not;
		}
		for (cur = end - 1; cur >= start; cur--)
		{
			if (*cur != pattern_not)
			{
				if (errors == NULL)
				{
					errors = init_list();
				}
				add_error(errors,(def_int_t)start,sizeof(def_int_t)*(cur - start),pattern_not,(def_int_t)*cur);
			}
			*cur = pattern;
		}
	}

	return errors;
}
