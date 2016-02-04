/*
    Copyright (C) 2010  Jens Neuhalfen -- Some functions have been extracted from
    the Linux kernel. These have been noted as such.

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
/**
 * This module provides the user space direct access to physical memory.
 * In this it mimics the behaviour of /dev/mem.
 *
 * See phys_mem.h for a complete documentation!
 *
 *
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
#include <linux/mmzone.h>
/* #include <linux/page-flags.h> */
#include <linux/pageblock-flags.h>
#include <linux/page-isolation.h>

#include "phys_mem.h"           /* local definitions */
#include "phys_mem_int.h"           /* local definitions */
#include "page_claiming.h"           /* local definitions */

static struct page * claim_free_buddy_page(struct page *requested);
bool is_free_buddy_page(struct page *page);
static int prep_new_page(struct page *page, int order);

static void __attribute__((unused)) dumpinfo(struct page *requested_page)
{
	pr_err("PHYDEBUG:pfn %08lx: headpfn %08lx count %d mapcount %d headcount %d flags %016lx headflags %016lx\n",
		page_to_pfn(requested_page),
		page_to_pfn(compound_head(requested_page)),
		page_count(requested_page),
		page_mapcount(requested_page),
		page_count(compound_head(requested_page)),
		requested_page->flags,
		compound_head(requested_page)->flags);
}

/*
 * Try to claim the requested page, returns 0 for
 * success or nonzero for error
 */
int do_try_claim_free_buddy_page(struct page *requested_page)
{
	struct page * locked_page = NULL;
	/* unsigned long pfn = page_to_pfn(requested_page); */
	unsigned int locked_page_count_after, locked_page_count_before;
	int ret = -1;

#if 0
	/* Failed rewrite attempt */
	if (0 == page_count(compound_head(requested_page))
	    && is_free_buddy_page(requested_page)) {
		/* 
		 * Page is free and a buddy page
		 * set_migratetype_isolate seems to set PageReserved
		 * which would cause is_free_buddy_page() to fail
		 */
		int smi = set_migratetype_isolate(requested_page);
		locked_page_count_before = page_count(requested_page);

		/* get, while page is still isolated */
		locked_page = claim_free_buddy_page(requested_page);

		/* 
                 * Note: This code proceeds even if isolating failed.
                 * The original never checked
		 * for the result and worked anyway.
		 */
		if (!smi)
			unset_migratetype_isolate(requested_page);
	}
#else
	/* Hacked version of original code without isolation */
	locked_page_count_before = page_count(requested_page);
	if (0 == page_count(compound_head(requested_page)))
		if (is_free_buddy_page(requested_page))
			/* get, while page is still isolated */
			locked_page = claim_free_buddy_page(requested_page);
#endif

	if (locked_page) {
		/* The page is now rightfully ours! */
		locked_page_count_after = page_count(locked_page);
		ret = 0;
	}
	return ret;
}

/*
 * Claim a given page from the buddy subsystem. This only works, if the page
 * registered within the buddy system and marked as free
 */
int try_claim_free_buddy_page(struct page* requested_page,
			      unsigned int allowed_sources,
                              struct page** allocated_page,
			      unsigned long* actual_source)
{
	int ret = CLAIMED_TRY_NEXT;

	if (allowed_sources & SOURCE_FREE_BUDDY_PAGE) {
		/*
		 * Isolate the page, so that it doesn't get reallocated if it
		 * was free.
		 */
    		if (!do_try_claim_free_buddy_page(requested_page)) {
			/* Claiming worked */
			ret = CLAIMED_SUCCESSFULLY;
#if 0
		/* shaking moved to hotplug */
		} else {
			/* Claiming failed, try to shake the page */
			shake_page(requested_page, 1);
			if (!do_try_claim_free_buddy_page(requested_page))
				ret = CLAIMED_SUCCESSFULLY;
#endif
#if 0
		} else {
			if (allowed_sources & SOURCE_SHAKING)
				dumpinfo(requested_page);
#endif
		}

		if (ret == CLAIMED_SUCCESSFULLY)
			*actual_source = SOURCE_FREE_BUDDY_PAGE;

	}
	return ret;
}

/*
 * Wrest a page from the buddy system.
 *
 * CAVE:
 *
 * This method manipulates buddy-system internal structures to accomplish this
 * goal.
 *
 * Source:
 * This methods implementation has been inspired by  "__rmqueue_smallest"
 */
static inline struct page *claim_free_buddy_page(struct page * requested)
{
	struct page* ret = NULL;
	unsigned int order = 0;
	struct zone *zone;
	int requested_page_count;
	unsigned long flags1, flags2; // yes, it's stupid - but should work

	zone = page_zone(requested);
	/* Protect the lru list */
	spin_lock_irqsave(&zone->lru_lock, flags1);

	/* Protect the area */
	spin_lock_irqsave(&zone->lock, flags2);

	requested_page_count = page_count(requested);

	if (likely(0 == requested_page_count) && PageBuddy(requested)) {
		unsigned int current_order;
		struct free_area * area;
		int migratetype;

		migratetype = get_pageblock_migratetype(requested);
		current_order = mm_page_order(requested);
		area = &(zone->free_area[current_order]);
		list_del(&requested->lru);
		mm_rmv_page_order(requested);
		area->nr_free--;
		mm_buddy_expand(zone, requested, order, current_order,
				area, migratetype);

		__mod_zone_page_state(zone, NR_FREE_PAGES, -1);

		ret = requested;
	} else {
#if 0
		pr_debug("NOT:  likely(0 == requested_page_count {%i}) && PageBuddy(requested){%s} \n",
			 requested_page_count, PageBuddy(requested) ? "true" : "false");
#endif
	}

	spin_unlock_irqrestore(&zone->lock, flags2);
	spin_unlock_irqrestore(&zone->lru_lock, flags1);

	if (ret && prep_new_page(ret, 0))
		pr_alert("Could not prep_new_page %p, %lu \n",
			 ret, page_to_pfn(ret));
	return ret;
}

static void bad_page(struct page *page)
{
	static unsigned long resume;
	static unsigned long nr_shown;
	static unsigned long nr_unshown;

	/* Don't complain about poisoned pages */
	if (PageHWPoison(page)) {
		__ClearPageBuddy(page);
		return;
	}

	/*
	 * Allow a burst of 60 reports, then keep quiet for that minute;
	 * or allow a steady drip of one report per second.
	 */
	if (nr_shown == 60) {
		if (time_before(jiffies, resume)) {
			nr_unshown++;
			goto out;
		}
		if (nr_unshown) {
			pr_alert("BUG: Bad page state: %lu messages suppressed\n",
				 nr_unshown);
			nr_unshown = 0;
		}
		nr_shown = 0;
	}

	if (nr_shown++ == 0)
		resume = jiffies + 60 * HZ;

	pr_alert("BUG: Bad page state in process   pfn:%05lx\n",
		 page_to_pfn(page));
	/* dump_page(page); */

	dump_stack();
out:
	/* Leave bad fields for debug, except PageBuddy could make trouble */
	__ClearPageBuddy(page);
	add_taint(TAINT_BAD_PAGE);
}

/*
 * This page is about to be returned from the page allocator
 */
static inline int check_new_page(struct page *page)
{
	if (unlikely(page_mapcount(page) |
	    (page->mapping != NULL) |
	    (atomic_read(&page->_count) != 0) |
	    (page->flags & PAGE_FLAGS_CHECK_AT_PREP))) {
		bad_page(page);
		return 1;
	}
	return 0;
}

static inline void set_page_count(struct page *page, int v)
{
	atomic_set(&page->_count, v);
}

/*
 * Turn a non-refcounted page (->_count == 0) into refcounted with
 * a count of one.
 */
static inline void set_page_refcounted(struct page *page)
{
	VM_BUG_ON(PageTail(page));
	VM_BUG_ON(atomic_read(&page->_count));
	set_page_count(page, 1);
}

static int prep_new_page(struct page *page, int order)
{
	int i;

	for (i = 0; i < (1 << order); i++) {
		struct page *p = page + i;
		if (unlikely(check_new_page(p)))
			return 1;
	}

	set_page_private(page, 0);
	set_page_refcounted(page);
#if 0
	arch_alloc_page(page, order);
	kernel_map_pages(page, 1 << order, 1);
	prep_zero_page(page, order, gfp_flags);
#endif
	return 0;
}
