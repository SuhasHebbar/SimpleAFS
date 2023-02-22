pkill unreliablefs
rm ./*.log


lst=$(ls | grep .f$)
proot=$HOME/unreliablefs

rm -rf "$proot"/base/*
rm -rf "$proot"/mnt/*

"$proot"/build/unreliablefs/unreliablefs "$proot"/mnt -basedir="$proot"/base -d 2>stderr.log >stdout.log & 
pid=$!
sleep 10

echo the executable is "$proot"/build/unreliablefs/unreliablefs

function xv6_bench {
	cd "$proot"/mnt || exit
	git clone https://github.com/mit-pdos/xv6-public.git
	cd xv6-public

	# echo Ctrl + a and the x. Source: https://superuser.com/questions/1543533/how-can-i-simulate-keystrokes-in-bash-without-any-packages
	echo $'\cax' | make qemu-nox
} 

function leveldb_bench_setup {
	cd $proot/mnt

	git clone https://github.com/google/leveldb.git
	cd leveldb
	git submodule update --init
	mkdir build
	cd build
	cmake ..
}

function leveldb_bench {
	cd $proot/mnt
	make clean
	make -j "$1"
	make clean
}

rm -rf $proot/mnt/xv6-public
echo Starting xv6_bench
time ( xv6_bench > /dev/null 2>&1)
echo Ending xv6_bench

num_threads=(1 2 4 8 16)
rm -rf $proot/mnt/leveldb

echo Starting leveldb_bench_setup
time ( leveldb_bench_setup > /dev/null 2>&1 )
echo Ending leveldb_bench_setup

for i in "${num_threads[@]}"; do
	echo Starting leveldb_bench $i
	time ( leveldb_bench $i > /dev/null 2>&1 )
	echo Ending leveldb_bench $i
done

kill $pid
pkill unreliablefs

