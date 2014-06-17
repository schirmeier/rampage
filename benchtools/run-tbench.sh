#!/bin/bash

LOGPREFIX=$1
shift

tbench_srv &

while [ .$1 != . ]; do
  tbench $1 > $LOGPREFIX-$1
  shift
done

kill %1
