timestamp=$(date +%s)
cd ~/dirs
mv cache cache_$timestamp
mv tmp tmp_$timestamp
mkdir cache
mkdir tmp
