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
#include "phys_mem_int.h"           /* local definitions */
#include "page_claiming.h"           /* local definitions */

/**
 * Free pages using memory-hotplug to increase
 * the success rate of the buddy-claimer
 */

/* Number of pages to offline/online at once
 * Minimum is max(pageblock_nr_pages, 1<<MAX_ORDER), must be an integer
 * multiple of the minimum.
 */
#define OFFLINE_AT_ONCE (1 << MAX_ORDER)

/**
 * hotplug_bounce - unplug and replug a pageblock
 * @address: start address of the pageblock
 *
 * This function tries to unplug and replug a pageblock starting at
 * @address. Returns nonzero on error.
 */
static int hotplug_bounce(u64 address)
{
	int ret;

	ret = remove_memory(address, OFFLINE_AT_ONCE << PAGE_SHIFT);
	if (!ret) {
		ret = online_pages(address >> PAGE_SHIFT, OFFLINE_AT_ONCE);
		if (ret) {
			pr_emerg("unclaim failed for pfn %08llx: ret %d\n",
				 address >> PAGE_SHIFT, ret);
		}
		ret = 0; /* avoid retries if onlining failed */
	}
	return ret;
}

static u64 last_addr = -1;

int free_pages_via_hotplug(struct page *requested_page,
			   unsigned int allowed_sources,
			   struct page **allocated_page,
			   unsigned long *actual_source)
{
	if (allowed_sources & SOURCE_HOTPLUG) {
		unsigned long pfn = page_to_pfn(requested_page);
		u64 address = pfn << PAGE_SHIFT;

#if 0
		/* Hack for testing - concentrate on just one 128MB block */
		if (page_to_pfn(requested_page) < (0x8000000 >> PAGE_SHIFT))
			return CLAIMED_TRY_NEXT;
		if (page_to_pfn(requested_page) >= (0x10000000 >> PAGE_SHIFT))
			return CLAIMED_TRY_NEXT;
#endif

		u64 realaddress = address &
			~(((unsigned long)OFFLINE_AT_ONCE << PAGE_SHIFT) - 1UL);

		/* Avoid claiming the same area twice in a row */
		if (realaddress != last_addr) {
			last_addr = realaddress;

			/* we try again only when remove_memory(offline) failed */
			if (hotplug_bounce(realaddress)) {
				if (allowed_sources & SOURCE_SHAKING) {
					/* Failed, shake page and try again */
					shake_page(requested_page, 1);
					hotplug_bounce(realaddress);
					/* ignore return value, we only retry once */
				}
			}
		}
	}

	/* Always fall through to the next claimer */
	return CLAIMED_TRY_NEXT;
}

