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
wait $server_pid
