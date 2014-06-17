#!/bin/bash

SPARE_MBYTES=150

if [ $# -lt 1 ]; then
	SLEEPTIME=3600
else
	SLEEPTIME=$1
fi

MBYTES=$(free -m| awk '/Mem:/ { print $2 }')
MBYTES=$(($MBYTES - $SPARE_MBYTES))
$(dirname $0)/memhog2 $MBYTES $SLEEPTIME
