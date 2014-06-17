#!/bin/bash

echo Filling cache ...
find ~ -type f|xargs cat > /dev/null 2>/dev/null &

OLDFREE=99999999
while true; do
	sleep 5
	FREE=$(free | awk '/Mem:/ { print $4 }')
	if [ $OLDFREE -le $FREE ]; then
		kill %1
		echo Done.
		break
	fi
	OLDFREE=$FREE
done
