pkill unreliablefs
rm *.log

lst=$(ls | grep .f$)
ubase=$HOME/unreliablefs
$ubase/build/unreliablefs/unreliablefs $ubase/mnt -basedir=$ubase/base -d 2>stderr.log >stdout.log & 
pid=$!

echo the executable is $ubase/build/unreliablefs/unreliablefs
j=$((0))
for i in $lst
do 
	j=$((j + 1))
	echo "($j)================================================================================================"
	filebench -f $i
	echo ================================================================================================
done

kill $pid
pkill unreliablefs

