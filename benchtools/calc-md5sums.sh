#!/bin/bash

date
echo Creating HD activity
find / -xdev -type f 2> /dev/null | xargs md5sum > /dev/null 2> /dev/null
echo MD5 calculation done
date
