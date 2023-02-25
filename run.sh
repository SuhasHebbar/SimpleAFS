umount $HOME/dirs/mnt
pkill suhas_server
pkill suhas_client

cp ./debug/unreliablefs/server ./debug/unreliablefs/suhas_server
cp ./debug/unreliablefs/client ./debug/unreliablefs/suhas_client

rm -rf $HOME/dirs/cache/*
rm -rf $HOME/dirs/tmp/*
rm -rf $HOME/dirs/mnt/*

./debug/unreliablefs/suhas_server $HOME/dirs/cache 127.0.0.1:8761 &
server_pid=$! || kill 0
# ./debug/unreliablefs/client $HOME/dirs/mnt -cachedir=$HOME/dirs/tmp -d >stdout.log 2> stderr.log &
./debug/unreliablefs/suhas_client $HOME/dirs/mnt -cachedir=/tmp/hebbar2 -serveraddr=127.0.0.1:8761 -d 2>/dev/null &
client_pid=$! || kill 0
wait $client_pid
wait $server_pid

