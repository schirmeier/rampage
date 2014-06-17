#!/bin/bash

LOGPREFIX=$1
TESTNAME=$2
RUNTIME=$3

if [ "$RUNTIME" == "" ]; then
    RUNTIME=120
fi

# delay before starting to ensure that memtester has started testing
echo Sleeping before running netperf...
sleep 60

echo Running netperf...
netperf -l $RUNTIME -t $TESTNAME -H 127.0.0.1 > $LOGPREFIX-$TESTNAME
