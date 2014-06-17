cat <<EOT
REPODIR=/home/ingo/memtester/rampage
SCRIPTDIR=<REPODIR>/benchtools
INFINITESCRIPTS=<SCRIPTDIR>/infinite
KERNELDIR=/home/ingo/memtester/bench/linux-2.6.35

IOZONEARGS=1500m 4k 8k 16k 32k 64k 128k 256k 512k
COREMARKARGS=/home/ingo/memtester/bench/coremark_v1.0 4 1000000
TBENCHARGS=1 32
MTARGS_FULL=-a blockwise --run-time 86400 --retest-time 2 -4 -t mt86\* -f 100
MTARGS_SLOW=-a slow --run-time 86400 --retest-time 2 -4 -t mt86\* -f 100 --frames-per-check 1024
KERNELCONFIG=allmodconfig
KERNELPARALLEL=8

CLEANUP=<SCRIPTDIR>/cleanup-logs.sh
SLEEPTIME=3600

ENERGY_SERVER=129.217.43.53
ENERGY_PORT=4444

run mkdir -p <RESULTDIR>
EOT

memtester() {
	local FILENAME=$1
	local MTARGS=$2
	local CLAIMER=$3
	echo -n "run <SCRIPTDIR>/start-memtest.sh <REPODIR> <$MTARGS> --logfile <RESULTDIR>/framelog-$FILENAME --enable-p4-claimers="
	case $CLAIMER in
	b) echo "buddy" ;;
	bh) echo "buddy,hotplug-claim" ;;
	bhs) echo "buddy,hotplug-claim,shaking" ;;
	esac
}

bench_init() {
	local COMMENT=$1
	echo "# $COMMENT"
	echo "reboot"
	echo "run <CLEANUP>"
}

energy() {
	local COMMAND=$1
	local FILENAME=$2
	echo "run <SCRIPTDIR>/energy-client.sh <ENERGY_SERVER> <ENERGY_PORT> $COMMAND $FILENAME"
}

bench_run() {
	local BENCH=$1
	local FILENAME=$2
	case $BENCH in
	none) echo "run sleep <SLEEPTIME>" ;;
	kcompile) echo "run <SCRIPTDIR>/run-compile-once.sh <KERNELDIR> <KERNELCONFIG> <KERNELPARALLEL> <RESULTDIR>/$FILENAME" ;;
	pov) echo "run <SCRIPTDIR>/run-pov.sh <RESULTDIR>/$FILENAME" ;;
	iozone) echo "run <SCRIPTDIR>/run-iozone.sh <RESULTDIR>/$FILENAME <IOZONEARGS>" ;;
	coremark) echo "run <SCRIPTDIR>/run-coremark.pl <COREMARKARGS> <RESULTDIR>/$FILENAME" ;;
	tbench) echo "run <SCRIPTDIR>/run-tbench.sh <RESULTDIR>/$FILENAME <TBENCHARGS>" ;;
	esac
}

bench_run_infinite() {
	local BENCH=$1
	# ignored, as we are not interested in actual benchmark results:
	local FILENAME=$2
	case $BENCH in
	none) echo "run sleep <SLEEPTIME>" ;;
	kcompile) echo "run <INFINITESCRIPTS>/run-compile-once.sh <SLEEPTIME> <KERNELDIR> <KERNELCONFIG> <KERNELPARALLEL>" ;;
	pov) echo "run <INFINITESCRIPTS>/run-pov.sh <SLEEPTIME>" ;;
	iozone) echo "run <INFINITESCRIPTS>/run-iozone.sh <SLEEPTIME> <IOZONEARGS>" ;;
	coremark) echo "run <INFINITESCRIPTS>/run-coremark.pl <COREMARKARGS> <SLEEPTIME>" ;;
	tbench) echo "run <INFINITESCRIPTS>/run-tbench.sh <SLEEPTIME> <TBENCHARGS>" ;;
	esac
}

bench_finish() {
	local FILENAME=$1
	echo "run date > <RESULTDIR>/$FILENAME-OK"
	echo ""
}
