#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 timelimit"
    exit 1
fi

# timeout
TIMELIMIT=$1
shift

Cleanup () {
    kill %1 2> /dev/null # kill sleeper process
    kill %2 2> /dev/null # kill povray
}

trap "exit" INT TERM
trap "Cleanup" EXIT

# start timeout subprocess
EXITFLAG=0
PARENTPID=$$
trap "EXITFLAG=1" ALRM
( sleep $TIMELIMIT ; kill -ALRM $PARENTPID ) &

LOOP=1

while [ $EXITFLAG == 0 ]; do
    echo "Running povray (loop $LOOP)"
    # FIXME: Yuck.
    cd /home/ingo/.phoronix-test-suite/installed-tests/povray
    export LOG_FILE=/dev/null
    ./povray

    LOOP=`expr $LOOP + 1`
done

echo "exiting due to timeout"

