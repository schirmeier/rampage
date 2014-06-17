#ifndef _ALGO_H_
#define _ALGO_H_ 1

#include <sys/types.h>
#include "error.h"
#include "config.h"

/*
 * Alle Tests beschreiben *immer* ein ganzes Wort (32 vs 64 bit) Speicher
 */

/* Test #0 
 * Braucht mindestens 128k Speicher!
 */
struct list_head* test_address_bits(def_int_t *pStart,def_int_t *pEnd,def_int_t pPattern);
/* Test #1 */
struct list_head* test_address_own(def_int_t *pStart,def_int_t *pEnd);
/* Test #2 */
struct list_head* test_mv_inv_onezeros(def_int_t *pStart, def_int_t *pEnd);
/* Test #3 */
struct list_head* test_mv_inv_eightpat(def_int_t *pStart, def_int_t *pEnd);
/* Test #4 */
struct list_head* test_mv_inv_rand(def_int_t *pStart, def_int_t *pEnd);
/* Test #5 */
struct list_head* test_block_move(def_int_t *pStart, def_int_t *pEnd);
/* Test #6 */
struct list_head* test_mv_inv_32(def_int_t *pStart, def_int_t *pEnd);
/* Test #7 */
struct list_head* test_random_numbers(def_int_t *pStart, def_int_t *pEnd);
/* Test #8 */
struct list_head* test_mod_20(def_int_t *pStart, def_int_t *pEnd);

#endif
