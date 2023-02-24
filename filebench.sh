
lst=$(ls bench/filebench_workloads | grep .f$)

j=$((0))
for i in $lst
do 
	j=$((j + 1))
	echo "($j)================================================================================================"
	echo Running $i
	filebench -f bench/filebench_workloads/$i
	echo ================================================================================================
done

