#!/usr/bin/bash

cd /wazuhl-build
cmake -G Ninja /wazuhl -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/llvm
ninja
ninja install

cd /
pip3 install -r /wazuhl/training_ground/requirements.txt
python3 /wazuhl/training_ground/train.py /llvm/bin/clang /wazuhl/suites/llvm-test-suite /wazuhl/suites/llvm-test-suite-build /caffe/build/lib
