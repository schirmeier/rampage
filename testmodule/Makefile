obj-m := phys_dummy.o

phys_dummy-objs := main.o file_operations.o  mmap_phys.o
phys_dummy-objs += page_claiming/page_claiming.o
phys_dummy-objs += page_claiming/pseudo_claiming.o

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

