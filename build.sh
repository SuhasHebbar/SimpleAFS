# cmake -S . -B debug -DCMAKE_INSTALL_PREFIX=/users/hebbar2/.local/bin -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON; cmake --build debug --parallel
# cmake -S . -B debug -DCMAKE_INSTALL_PREFIX=/users/hebbar2/.local/bin -DCMAKE_BUILD_TYPE=RelWithDebInfo; cmake --build debug --parallel
# cmake -S . -B debug -DCMAKE_INSTALL_PREFIX=/users/hebbar2/.local/bin -DCMAKE_BUILD_TYPE=Debug; cmake --build debug --parallel
cmake -S . -B debug -DCMAKE_INSTALL_PREFIX=/users/hebbar2/.local/bin -DCMAKE_BUILD_TYPE=Release; cmake --build debug --parallel
