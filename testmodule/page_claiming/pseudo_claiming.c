#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>

#include "phys_mem.h"
#include "phys_mem_int.h"
#include "page_claiming.h"

static unsigned long block_first_pfn;
static unsigned long block_order = 0;

void pseudoclaimer_init(void) {
  struct page *startpage;
  unsigned int order = MAX_ORDER;

  /* try to allocate the largest block of pages that we can get */
  do {
    startpage = alloc_pages(GFP_KERNEL | __GFP_NOWARN | __GFP_NORETRY, order);
    if (startpage == NULL)
      order--;
  } while (order != 0 && startpage == NULL);

  block_first_pfn = page_to_pfn(startpage);
  block_order     = order;

  printk(KERN_INFO "pseudoclaimer allocated an order %d block at pfn %08lx\n",
         order, (unsigned long)block_first_pfn);
}

void pseudoclaimer_free(void) {
  if (block_order > 0) {
    __free_pages(pfn_to_page(block_first_pfn), block_order);
    block_order = 0;
  }
}

int try_claim_pages_pseudo(struct page* requested_page,
                           unsigned int allowed_sources,
                           struct page** allocated_page,
                           unsigned long *actual_source) {
  unsigned long pfn = page_to_pfn(requested_page);
  if (block_order > 0 &&
      pfn >= block_first_pfn &&
      pfn < (block_first_pfn + (1 << block_order))) {
    /* within memory block */

    /* return one of the allowed_sources as actual source */
    int i;
    for (i=0; i<32; i++) {
      if (allowed_sources & (1 << i)) {
        *actual_source = 1 << i;
        break;
      }
    }

    return CLAIMED_SUCCESSFULLY;
  } else {
    /* fail */
    return CLAIMED_TRY_NEXT;
  }
}
