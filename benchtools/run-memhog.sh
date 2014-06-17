#!/bin/bash

cd $(dirname $0)

if [ ! -x memhog ]
then
	gcc -Wall -o memhog memhog.c
fi

echo Running memhog ...
./memhog > $1
