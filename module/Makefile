obj-m := phys_mem.o

phys_mem-objs := main.o file_operations.o  mmap_phys.o
phys_mem-objs += page_claiming/page_claiming.o

phys_mem-objs += page_claiming/free_page_claiming.o
phys_mem-objs += page_claiming/free_buddy_page_claiming.o
phys_mem-objs += page_claiming/page_cache_claiming.o
phys_mem-objs += page_claiming/anon_page_claiming.o
phys_mem-objs += page_claiming/helper.o
phys_mem-objs += page_claiming/difficult_pages.o
phys_mem-objs += page_claiming/hotplug_freeing.o
phys_mem-objs += page_claiming/hotplug_claiming.o


VERSION ?= $(shell uname -r)

EXTRA_CFLAGS += -I$(PWD)/include

KDIR  := /lib/modules/$(VERSION)/build
PWD   := $(shell pwd)


.PHONY: all
all: clean module

module:
	echo $(EXTRA_CFLAGS)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE)  -C $(KDIR) M=$(PWD) clean

test: module
	sudo phys_mem.ko
	sudo ../test/tests.sh || true
	sudo rmmod phys_mem
