#!/bin/bash
echo Starting setup script

chmod 400 ~/.ssh/id_ed25519*

# Disable ASLR for filebench
# Value was 2 previously
sudo bash -c "echo 0 > /proc/sys/kernel/randomize_va_space"

if [[ "" == "$(ls setup_19.x)" ]]; then
	curl -fsSL https://deb.nodesource.com/setup_19.x -O 
	sudo -E bash setup_19.x
fi

# sudo apt update
# sudo apt autoremove -y --purge
sudo apt install -y htop fuse libfuse-dev build-essential autoconf libtool pkg-config cmake qemu-system flex bison python-is-python3 python3-paramiko nodejs npm hyperfine ripgrep

if [[ ":$PATH:" != *":$HOME/bin:"* ]]; then
	echo Adding \$HOME/bin to path
	mkdir -p ~/bin
	echo "export PATH=\$HOME/bin:\$PATH" >> ~/.bashrc
fi

if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
	echo Adding \$HOME/.local/bin to path
	mkdir -p ~/bin
	echo "export PATH=\$HOME/.local/bin:\$PATH" >> ~/.bashrc
fi

if [[ "" == "$(ls -ald fs)" ]]; then
	git clone git@github.com:SuhasHebbar/CS739-P1.git fs
fi

# if [[ ":$PATH:" != *":$HOME/go/bin:"* ]]; then
# 	echo Adding \$HOME/go/	echo Adding \$HOME/bin to pathbin to path
# 	mkdir -p ~/go/bin
# 	echo "export PATH=\$HOME/go/bin:\$PATH" >> ~/.bashrc
# fi

# if [[ "x" == "x$(which protoc-gen-go)" || "x" == "x$(which protoc-gen-go-grpc)" ]]; then
# 	go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
# 	go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2
# fi


if [[ "" == "$(which nvim)" ]]; then
	curl -L https://github.com/neovim/neovim/releases/latest/download/nvim.appimage -o ~/bin/nvim
	chmod u+x ~/bin/nvim
fi

if [[ "" == "$(ls -ald ~/filebench)" ]]; then
	# We're using the alpha version because the latest stable version does not have the ./configure script.
	curl -L https://github.com/filebench/filebench/releases/download/1.5-alpha3/filebench-1.5-alpha3.zip -o ~/filebench.zip
	unzip filebench.zip
	mv filebench-1.5-alpha3 filebench
	rm -rf filebench.zip
	pushd ~/filebench || exit
	./configure --prefix="$HOME/.local"
	make
	make install
	popd || exit

fi

if [[ "" == "$(ls -ald grpc)" ]]; then
	MY_INSTALL_DIR=~/.local/bin
	mkdir -p $MY_INSTALL_DIR
	git clone --recurse-submodules -b v1.50.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
	pushd grpc || exit

	mkdir -p cmake/build
	pushd cmake/build || exit

	cmake -DgRPC_INSTALL=ON \
	      -DgRPC_BUILD_TESTS=OFF \
	      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
	      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	      ../..
	make -j 20
	make install
	popd || exit

	popd || exit

fi

if [[ "" == "$(ls -ald bench)" ]]; then
	curl -L https://github.com/jingliu9/unreliablefs/archive/refs/heads/cs739.zip -O
	unzip cs739.zip
	rm -rf cs739.zip
	mv unreliablefs-cs739 bench
fi

if [[ "" == "$(ls -ald xv6-public)" ]]; then
	git clone https://github.com/mit-pdos/xv6-public.git
fi


if [[ "" == "$(ls -ald leveldb)" ]]; then
	git clone https://github.com/google/leveldb.git
	pushd leveldb || exit

	git submodule update --init
	mkdir build
	pushd build || exit

	cmake ..
	make clean
	popd || exit
	popd || exit

fi

rm -rf ~/.config/nvim
rm -rf ~/hebbar2_config

git clone https://github.com/SuhasHebbar/config hebbar2_config
ln -s ~/hebbar2_config/nvim ~/.config/nvim

git config --global user.name "Group 1"
git config --global user.email "group1@not_a_real_email.com"

# sudo apt remove --purge -y neovim clangd cmake
# rm -rvf ~/grpc
echo Finished setup script
