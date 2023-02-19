pkill unreliablefs
rm ./*.log


lst=$(ls | grep .f$)
ubase=$HOME/unreliablefs

rm -rf $ubase/base/*
rm -rf $ubase/mnt/*

$ubase/build/unreliablefs/unreliablefs $ubase/mnt -basedir=$ubase/base -d 2>stderr.log >stdout.log & 
pid=$!
sleep 10

echo the executable is $ubase/build/unreliablefs/unreliablefs

function xv6_bench {
	cd $ubase/mnt
	git clone https://github.com/mit-pdos/xv6-public.git
	cd xv6-public

	# echo Ctrl + a and the x. Source: https://superuser.com/questions/1543533/how-can-i-simulate-keystrokes-in-bash-without-any-packages
	echo $'\cax' | make qemu-nox
} 

function leveldb_bench_setup {
	cd $ubase/mnt

	git clone https://github.com/google/leveldb.git
	cd leveldb
	git submodule update --init
	mkdir build
	cd build
	cmake ..
}

function leveldb_bench {
	cd $ubase/mnt
	make clean
	make -j "$1"
	make clean
}

rm -rf $ubase/mnt/xv6-public
echo Starting xv6_bench
time ( xv6_bench > /dev/null 2>&1)
echo Ending xv6_bench

ts=(1 2 4 8 16)
rm -rf $ubase/mnt/leveldb

echo Starting leveldb_bench_setup
time ( leveldb_bench_setup > /dev/null 2>&1 )
echo Ending leveldb_bench_setup

for i in "${ts[@]}"; do
	echo Starting leveldb_bench $i
	time ( leveldb_bench $i > /dev/null 2>&1 )
	echo Ending leveldb_bench $i
done

kill $pid
pkill unreliablefs

