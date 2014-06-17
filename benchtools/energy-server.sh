#!/bin/bash
set -e

if [ $# -lt 3 ]; then
	echo "usage: $0 tcp-port sparometer-device logfile" >&2
	exit 1
fi

TCPPORT=$1
# must be lowercase: the sparometer tool directly expects this variable
export sparometer=$2
LOGFILE=$3
SPAROMETER_BINARY=$(dirname $0)/sparometer-0.0.4/sparometer
if [ ! -x "$SPAROMETER_BINARY" ]; then
	echo "$SPAROMETER_BINARY not executable" >&2
	exit 1
fi

NETCAT=nc.openbsd

sparometer_wrapper()
{
	OUT=ERROR
	while [[ $OUT == *ERROR* ]]; do
#		if [ "$OUT" != "ERROR" ]
#		then
#			echo "FAIL: $OUT" >&2
#		fi
		OUT=$("$SPAROMETER_BINARY" "$@" 2>&1)
	done
	[ -n "$OUT" ] && echo "$OUT" || true
}

echo "$(basename $0) -- port $TCPPORT, device $sparometer, logfile $LOGFILE"
[ -e "$LOGFILE" ] && echo "warning: $LOGFILE exists, appending" || true

( while true; do $NETCAT -kl $TCPPORT || true; done ) | \
while read line; do
	BENCHNAME=$(echo $line | awk '{print $2}')
	# command: restart benchmark-name
	if [[ $line == "restart "* ]]; then
		sparometer_wrapper --reset-log --start-log
		echo "$(date +%s) restart $BENCHNAME" | tee -a "$LOGFILE"
	# command: takekwh benchmark-name
	elif [[ $line == "takekwh "* ]]; then
		KWH=$(sparometer_wrapper --get-kwh | awk '{print $1}')
		NOW=$(date +%s)
		echo "$NOW kwh $BENCHNAME $KWH" | tee -a "$LOGFILE"
		sparometer_wrapper --get-logdata > "logdata-$BENCHNAME-$NOW-$KWH.txt"
	else
		echo "$(date +%s) UNKNOWN: $line" | tee -a "$LOGFILE"
	fi
done
