#!/bin/bash
sudo chmod 444 /proc/kpage*
sudo rmmod phys_dummy
make && sudo insmod phys_dummy.ko && sudo chmod 777 /dev/phys_mem && echo OK
