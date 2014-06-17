#!/bin/bash

echo Running povray...
cd /home/ingo/.phoronix-test-suite/installed-tests/povray
export LOG_FILE=$1
./povray
