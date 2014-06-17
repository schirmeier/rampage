# Sammlung aller gefundenen Benchmarks

SCRIPTDIR=/home/ingo/memtester/rampage/benchtools
KERNELDIR=/home/ingo/memtester/bench/linux-2.6.35
RESULTDIR=/home/ingo/memtester/results/alles

IOZONEARGS=1500m 4k 8k 16k 32k 64k 128k 256k 512k
MTARGS=-a blockwise --run-time 86400 --retest-time 2 -4 -t mt86\* -f 100
MTARGS_SLOW=-a slow --run-time 86400 --retest-time 2 -4 -t mt86\* -f 100
MTARGS_SLOW2=-a slow --run-time 86400 --retest-time 2 -4 -t mt86\* -f 100 --frames-per-check 1024
KERNELCONFIG=allmodconfig

CLEANUP=<SCRIPTDIR>/cleanup-logs.sh

iterate 5000
  ### iozone
  # Test gegen idlende Maschine
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-no-<ITERATION> <IOZONEARGS>

  # Test gegen cpuburn
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-cpuburn.sh
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-burn-<ITERATION> <IOZONEARGS>

  # Test gegen memtester auf "full power", nur buddy
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-b --enable-p4-claimers=buddy
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memb-<ITERATION> <IOZONEARGS>

  # Test gegen memtester auf "full power", Hotplug-Claimer
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-hb --enable-p4-claimers=hotplug-claim,buddy
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memhb-<ITERATION> <IOZONEARGS>

  # Test gegen memtester auf "full power", Buddy+Hotplug-Claimer+Shaking
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-hbs --enable-p4-claimers=hotplug-claim,buddy,shaking
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memhbs-<ITERATION> <IOZONEARGS>

  # Test gegen memtester "slow", nur buddy
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-slowb --enable-p4-claimers=buddy
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memslowb-<ITERATION> <IOZONEARGS>

  # Test gegen memtester "slow", Hotplug-Claimer
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-slowhb --enable-p4-claimers=hotplug-claim,buddy
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memslowhb-<ITERATION> <IOZONEARGS>

  # Test gegen memtester "slow", Buddy+Hotplug-Claimer+Shaking
  reboot
  run <CLEANUP>
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW> --logfile <RESULTDIR>/framelog-<ITERATION>-iozone-slowhbs --enable-p4-claimers=hotplug-claim,buddy,shaking
  run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/iozone-memslowhbs-<ITERATION> <IOZONEARGS>



  ### Apache-Bench
  # Test gegen idlende Maschine
  reboot
  run <SCRIPTDIR>/run-ab-all.sh <RESULTDIR>/ab-no-<ITERATION>

  # Test gegen cpuburn
  reboot
  run <SCRIPTDIR>/start-cpuburn.sh
  run <SCRIPTDIR>/run-ab-all.sh <RESULTDIR>/ab-burn-<ITERATION>

  # Test gegen memtester auf "full power"
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage -1 -a blockwise --run-time 2 --retest-time 2 --logfile <RESULTDIR>/framelog-<ITERATION>-ab
  run <SCRIPTDIR>/run-ab-all.sh <RESULTDIR>/ab-mem-<ITERATION>


  ### tbench
  # Test gegen idlende Maschine
  reboot
  run <SCRIPTDIR>/run-tbench.sh <RESULTDIR>/tbench-no-<ITERATION> 1 128

  # Test gegen cpuburn
  reboot
  run <SCRIPTDIR>/start-cpuburn.sh
  run <SCRIPTDIR>/run-tbench.sh <RESULTDIR>/tbench-burn-<ITERATION> 1 128

  # Test gegen memtester auf "full power"
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage -1 -a blockwise --run-time 2 --retest-time 2 --logfile <RESULTDIR>/framelog-<ITERATION>-tbench
  run <SCRIPTDIR>/run-tbench.sh <RESULTDIR>/tbench-mem-<ITERATION> 1 128


  ### netperf
  # Test gegen idlende Maschine
  reboot
  run <SCRIPTDIR>/run-netperf.sh <RESULTDIR>/netperf-no-<ITERATION> TCP_RR

  # Test gegen cpuburn
  reboot
  run <SCRIPTDIR>/start-cpuburn.sh
  run <SCRIPTDIR>/run-netperf.sh <RESULTDIR>/netperf-burn-<ITERATION> TCP_RR
 
  # Test gegen memtester auf "full power"
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage -1 -a blockwise --run-time 2 --retest-time 2 --logfile <RESULTDIR>/framelog-<ITERATION>-netperf
  run <SCRIPTDIR>/run-netperf.sh <RESULTDIR>/netperf-mem-<ITERATION> TCP_RR



  ### Kernelcompilation
  # Test gegen idlende Maschine
  reboot
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-no-<ITERATION>

  # Test gegen cpuburn
  reboot
  run <SCRIPTDIR>/start-cpuburn.sh
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-burn-<ITERATION>

  # Test gegen memtester auf "full power", nur buddy
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-b --enable-p4-claimers=buddy
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memb-<ITERATION>

  # Test gegen memtester auf "full power", Hotplug-Claimer
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-hb --enable-p4-claimers=hotplug-claim,buddy
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memhb-<ITERATION>

  # Test gegen memtester auf "full power", Buddy+Hotplug-Claimer+Shaking
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-hbs --enable-p4-claimers=hotplug-claim,buddy,shaking
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memhbs-<ITERATION>

  # Test gegen memtester "slow", nur buddy
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-slowb --enable-p4-claimers=buddy
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memslowb-<ITERATION>

  # Test gegen memtester "slow", Hotplug-Claimer
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-slowhb --enable-p4-claimers=hotplug-claim,buddy
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memslowhb-<ITERATION>

  # Test gegen memtester "slow", Buddy+Hotplug-Claimer+Shaking
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-compile-slowhbs --enable-p4-claimers=hotplug-claim,buddy,shaking
  run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> 8 <RESULTDIR>/kcompile-memslowhbs-<ITERATION>


  ### povray

  # Test gegen memtester "slow", nur buddy
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-pov-slowb --enable-p4-claimers=buddy
  run <SCRIPTDIR>/run-pov.sh <RESULTDIR>/pov-memslowb-<ITERATION>

  # Test gegen memtester "slow", Hotplug-Claimer
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-pov-slowhb --enable-p4-claimers=hotplug-claim,buddy
  run <SCRIPTDIR>/run-pov.sh <RESULTDIR>/pov-memslowhb-<ITERATION>

  # Test gegen memtester "slow", Buddy+Hotplug-Claimer+Shaking
  reboot
  run <SCRIPTDIR>/start-memtest.sh /home/ingo/memtester/rampage <MTARGS_SLOW2> --logfile <RESULTDIR>/framelog-<ITERATION>-pov-slowhbs --enable-p4-claimers=hotplug-claim,buddy,shaking
  run <SCRIPTDIR>/run-pov.sh <RESULTDIR>/pov-memslowhbs-<ITERATION>


enditer
