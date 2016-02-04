/*
    Copyright (C) 2010  Jens Neuhalfen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/memory_hotplug.h>
#include <linux/pageblock-flags.h>

#include "phys_mem.h"           /* local definitions */
#include "phys_mem_int.h"       /* local definitions */
#include "page_claiming.h"      /* local definitions */

/*
 * Free pages using memory-hotplug to increase the
 * success rate of the buddy-claimer
 */

/* Number of pages to offline/online at once
 * Minimum is max(pageblock_nr_pages, 1<<MAX_ORDER), must be an integer
 * multiple of the minimum.
 */
#define OFFLINE_AT_ONCE (1 << MAX_ORDER)

static u64 start_address = -1;
static u64 prev_start_address = -1;
static u64 prev_failed_claim = -1;

void unclaim_pages_via_hotplug(struct page* requested_page)
{
	unsigned long pfn = page_to_pfn(requested_page);
	u64 address = pfn << PAGE_SHIFT;
	u64 aligned_address = address &
			~(((unsigned long)OFFLINE_AT_ONCE << PAGE_SHIFT) - 1UL);
	int ret;

	/* Sanity check */
	if (aligned_address != start_address
	    && aligned_address != prev_start_address) {
		pr_crit("physmem: Attempted to unclaim_pages_via_hotplug on nonclaimed area!\n");
		return;
	}

	if (start_address != -1) {
		/* first attempt to unclaim in current block */
		/*
		 * FIXME: Technically the onlining should be delayed until all pages
		 *        claimed within the block are unclaimed (i.e. refcounting),
		 *        but at least in the "no errors" case this works because
		 *        memtester unclaims all pages at once
		 * NOTE: Onlinling only clears PageReserved (mm/memory_hotplug.c:online_page),
		 *       so any poison markers we added for bad pages stay active.
		 */
		ret = online_pages(aligned_address >> PAGE_SHIFT, OFFLINE_AT_ONCE);
		if (ret) {
			pr_crit("physmem: unclaim failed for pfn %08llx: ret %d\n",
				aligned_address >> PAGE_SHIFT, ret);
		}

		prev_start_address = start_address;
		start_address = -1;
	} else {
		/* unclaim in already-onlined block, do nothing */
		return;
	}
}

int try_claim_pages_via_hotplug(struct page* requested_page,
				unsigned int allowed_sources,
				struct page** allocated_page,
				unsigned long* actual_source)
{
	if (allowed_sources & SOURCE_HOTPLUG_CLAIM) {
		unsigned long pfn = page_to_pfn(requested_page);
		u64 address = pfn << PAGE_SHIFT;
		u64 aligned_address = address &
			~(((unsigned long)OFFLINE_AT_ONCE << PAGE_SHIFT) - 1UL);
		int ret;

		if (start_address != -1) {
			if (aligned_address == start_address) {
				/* in currently removed memory block */
				*actual_source = SOURCE_HOTPLUG_CLAIM;
				return CLAIMED_SUCCESSFULLY;
			} else {
				/* requested page not in current hotplug area */
				return CLAIMED_TRY_NEXT;
			}
		}

		/*
		 * Don't try to hotplug the same area twice
		 * if it failed the last time
		 */
		if (prev_failed_claim == aligned_address)
			return CLAIMED_TRY_NEXT;

		/* test: refuse to claim the first block of memory */
		if (aligned_address == 0)
			return CLAIMED_TRY_NEXT;

		ret = remove_memory(aligned_address,
				    OFFLINE_AT_ONCE << PAGE_SHIFT);
		if (ret) {
			if (allowed_sources & SOURCE_SHAKING) {
				/* Failed, shake page and try again */
				shake_page(requested_page, 1);
				ret = remove_memory(aligned_address,
						OFFLINE_AT_ONCE << PAGE_SHIFT);
				if (ret) {
					prev_failed_claim = aligned_address;
					return CLAIMED_TRY_NEXT;
				}
			} else {
				/* Failed, shaking not enabled */
				prev_failed_claim = aligned_address;
				return CLAIMED_TRY_NEXT;
			}
		}

		start_address = aligned_address;
		prev_start_address = -1;
		*actual_source = SOURCE_HOTPLUG_CLAIM;
		return CLAIMED_SUCCESSFULLY;
	}

	/* Always fall through to the next claimer */
 	return CLAIMED_TRY_NEXT;
}

