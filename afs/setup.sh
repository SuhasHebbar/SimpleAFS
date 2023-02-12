#!/usr/bin/env bash

set -ex

# Run from the root directory

## Install grpc
pushd ..
export LOCAL_INSTALL_DIR=$HOME/.local
mkdir -p $LOCAL_INSTALL_DIR
export PATH="$LOCAL_INSTALL_DIR/bin:$PATH"
git clone --recurse-submodules -b v1.50.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$LOCAL_INSTALL_DIR \
      ../..
make -j 4
make install
popd

# Install unreliablefs
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$LOCAL_INSTALL_DIR
cmake --build build --parallel
