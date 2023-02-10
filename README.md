Debug build:
cmake -S . -B debug -DCMAKE_INSTALL_PREFIX=$HOME/.local/bin -DCMAKE_BUILD_TYPE=Debug; cmake --build debug --parallel

Release build:
cmake -S . -B release -DCMAKE_INSTALL_PREFIX=$HOME/.local/bin -DCMAKE_BUILD_TYPE=Release; cmake --build release --parallel
