function cacheclr {
	timestamp=$(date +%s)
	pushd ~/dirs
	mv cache cache_$timestamp
	mv tmp tmp_$timestamp
	mkdir cache
	mkdir tmp
	popd
}

rm -rf ~/dirs/{cache,tmp}_* &

lst=$(ls bench/filebench_workloads | grep .f$)

j=$((0))
for i in $lst
do 
	echo "($j)================================================================================================"
	echo Running $i
	j=$((j + 1))
	cacheclr
	rm -rf $HOME/dirs/{cache,tmp}_* &

	time filebench -f ./bench/filebench_workloads/$i
	echo ================================================================================================
done

