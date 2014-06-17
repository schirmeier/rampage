#!/bin/bash
#
# Hilfsscript um einen Linuxkernel zwecks Benchmark genau einmal zu compilieren

if [ $# != 4 ]; then
  echo Usage: $0 kerneldir configtarget parallel_count outputfile
  exit 1
fi

cd "$1"
make mrproper && make $2 && /usr/bin/time --output="$4" make -j$3 all
