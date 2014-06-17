#!/bin/bash
sudo -A chmod 444 /proc/kpage*
sudo -A rmmod phys_mem
make && sudo -A insmod phys_mem.ko && sudo -A chmod 777 /dev/phys_mem && echo OK
sudo -A dmesg -n 1

