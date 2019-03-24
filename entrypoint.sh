#!/usr/bin/bash

cd /wazuhl-build
CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" cmake -G Ninja /wazuhl -DTorch_DIR=/libtorch/share/cmake/Torch/ -DLLVM_ENABLE_WAZUHL=1 -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_RTTI=ON -DCMAKE_INSTALL_PREFIX=/llvm -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_BUILD_RUNTIME=OFF -DLLVM_ENABLE_TERMINFO=OFF -DCLANG_ENABLE_ARCMT=OFF -DCLANG_ENABLE_STATIC_ANALYZER=OFF
ninja
ninja install

cd /
pip3 install -r /wazuhl/training_ground/requirements.txt
python3 /wazuhl/training_ground/train.py /llvm/bin/clang /wazuhl/suites/llvm-test-suite /wazuhl/suites/llvm-test-suite-build /libtorch/lib
