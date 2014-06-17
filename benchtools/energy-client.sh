#!/bin/bash
set -e

if [ $# -lt 4 ]; then
	echo "usage: $0 server tcp-port command bench-name" >&2
	echo "allowed commands: restart, takekwh" >&2
	echo "example: $0 129.217.43.53 4444 restart iozone-bhs-something" >&2
	exit 1
fi

SERVER=$1
TCPPORT=$2
COMMAND=$3
BENCHNAME=$4

NETCAT=nc.openbsd

echo "$(basename $0) -- telling $SERVER:$TCPPORT '$COMMAND $BENCHNAME'"

echo "$COMMAND $BENCHNAME" | $NETCAT $SERVER $TCPPORT
