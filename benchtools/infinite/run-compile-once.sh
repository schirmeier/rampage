#!/bin/bash

if [ $# != 4 ]; then
  echo Usage: $0 timelimit kerneldir configtarget parallel_count
  exit 1
fi

# cleanup and timeout
TIMELIMIT=$1
shift

Cleanup () {
    kill %1 # kill sleeper process
    echo "running mrproper"
    make mrproper
}

trap "exit" INT TERM
trap "Cleanup" EXIT

# start timeout subprocess
EXITFLAG=0
PARENTPID=$$
trap "EXITFLAG=1" ALRM
( sleep $TIMELIMIT ; kill -ALRM $PARENTPID ) &

LOOP=1

cd "$1"

while [ $EXITFLAG == 0 ]; do
    echo "Running compilation (loop $LOOP)"
    make mrproper && make $2 && make -j$3 all

    LOOP=`expr $LOOP + 1`
done

echo "exiting due to timeout in loop $LOOP"

