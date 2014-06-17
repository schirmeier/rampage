#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: $0 logfile filesize recordsize [recordsize...]"
    exit 1
fi

CMDLINE="-s $2 -i 0 -i 1"
LOGFILE="$1"

shift
shift

while [ $# -gt 0 ]; do
    CMDLINE="$CMDLINE -r $1"
    shift
done

/usr/bin/time iozone $CMDLINE > $LOGFILE 2>&1
