#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: $0 timelimit filesize recordsize [recordsize...]"
    exit 1
fi

# cleanup and timeout
TIMELIMIT=$1
shift

Cleanup () {
    kill %1 2> /dev/null # kill sleeper process
    kill %2 2> /dev/null # kill iozone
    [ -e iozone.tmp ] && rm -f iozone.tmp
}

trap "exit" INT TERM
trap "Cleanup" EXIT

# start timeout subprocess
EXITFLAG=0
PARENTPID=$$
trap "EXITFLAG=1" ALRM
( sleep $TIMELIMIT ; kill -ALRM $PARENTPID ) &

LOOP=1
CMDLINE="-s $2 -i 0 -i 1"
LOGFILE="$1"

shift
shift

while [ $# -gt 0 ]; do
    CMDLINE="$CMDLINE -r $1"
    shift
done

while [ $EXITFLAG == 0 ]; do
    echo "Running iozone (loop $LOOP)"
    iozone $CMDLINE

    LOOP=`expr $LOOP + 1`
done

echo "exiting due to timeout in loop $LOOP"

