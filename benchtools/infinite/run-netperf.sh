#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 timelimit testname"
    echo "WARNING: timelimit must be in seconds!"
    exit 1
fi

RUNTIME=$1
TESTNAME=$2

# delay before starting to ensure that memtester has started testing
echo Sleeping before running netperf...
sleep 60

echo Running netperf...
netperf -l $RUNTIME -t $TESTNAME -H 127.0.0.1 > /dev/null
