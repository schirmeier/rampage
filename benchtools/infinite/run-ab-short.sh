#!/bin/bash

if [ $# -ne 1 ]; then
    echo Usage: $0 timelimit
    exit 1
fi

# cleanup and timeout
TIMELIMIT=$1

Cleanup () {
    kill %1 # kill sleeper process
}

trap "exit" INT TERM
trap "Cleanup" EXIT

# start timeout subprocess
EXITFLAG=0
PARENTPID=$$
trap "EXITFLAG=1" ALRM
( sleep $TIMELIMIT ; kill -ALRM $PARENTPID ) &

export PATH=/usr/sbin:$PATH

LOOP=1

while [ $EXITFLAG == 0 ]; do
    echo "Running ab (loop $LOOP)"
    ab -n 100000 -c 30 http://localhost/index.html > /dev/null

    LOOP=`expr $LOOP + 1`
done

echo "exiting due to timeout"
