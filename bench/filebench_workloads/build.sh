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

cd $ubase/mnt

echo Starting xv6
git clone https://github.com/mit-pdos/xv6-public.git
cd xv6-public
make qemu-nox
echo Ending xv6

cd $ubase/mnt

echo Starting leveldb
git clone https://github.com/google/leveldb.git
cd leveldb
git submodule update --init
mkdir build
cd build
cmake ..
make clean
echo Ending leveldb

make -j 10

kill $pid
pkill unreliablefs

