#!/bin/bash

if [ $# -lt 2 ]; then
    echo Usage: $0 timelimit clientcount [clientcount...]
    exit 1
fi

TIMELIMIT=$1
shift
ARGS="$@"

# clean up if killed
Cleanup () {
    kill %1
    kill %2
}

trap "Cleanup; exit" INT TERM EXIT

# start server
tbench_srv &

# start timeout subprocess
EXITFLAG=0
trap "EXITFLAG=1" ALRM
PARENTPID=$$
( sleep $TIMELIMIT ; kill -ALRM $PARENTPID ) &

LOOP=1

while [ $EXITFLAG == 0 ]; do
    set $ARGS
    while [ .$1 != . -a $EXITFLAG == 0 ]; do
        echo "Running tbench with $1 clients (loop $LOOP)"
        tbench $1 > /dev/null
        shift
    done

    LOOP=`expr $LOOP + 1`
done

echo "exiting due to timeout"
