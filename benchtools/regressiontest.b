# run 1000 iterations of memtest with parallel HDD activity followed
# by 30 minutes pause - if it hangs or crashes something is still broken

SCRIPTDIR=/home/ingo/memtester/rampage/benchtools
KERNELDIR=/home/ingo/memtester/bench/linux-2.6.35

iterate 1000
  reboot
  run <SCRIPTDIR>/cleanup-logs.sh
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage -a blockwise --run-time 86400 -t linear-cext --frames-per-check=1024 -4 --enable-p4-claimers=hotplug-claim,shaking,buddy -f 5120 --retest-time 300
  run <SCRIPTDIR>/compile-once.sh <KERNELDIR> allnoconfig 8 /dev/null
  run <SCRIPTDIR>/calc-md5sums.sh
  run sleep 60m
enditer
