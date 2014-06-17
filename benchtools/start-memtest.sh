#!/bin/bash
#
# Kleines Hilfsscript zum Starten des Memtesters im Hintergrund

if [ $# -lt 2 ]; then
  echo Usage: $0 memtestroot main.py-params
  exit 1
fi

export SUDO_ASKPASS=/bin/true

MTROOT=$1
shift

cd $MTROOT/module
./notty-rebuild.sh
cd $MTROOT/userspace
rm -f /tmp/memtest_status
screen -S memtester -d -m $PWD/main.py $*
