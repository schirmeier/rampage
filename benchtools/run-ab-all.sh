#!/bin/bash

export PATH=/usr/sbin:$PATH

ab -n 100000       http://localhost/index.html > $1-c1-html
ab -n 100000 -c 30 http://localhost/index.html > $1-c30-html
ab -n 50000        http://localhost/test.php   > $1-c1-phpinfo
ab -n 50000  -c 30 http://localhost/test.php   > $1-c30-phpinfo
ab -n 50000        http://localhost/test2.php  > $1-c1-echo
ab -n 50000  -c 30 http://localhost/test2.php  > $1-c30-echo
