#!/usr/bin/env bash

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz
tar xzf mongo-c-driver-1.13.0.tar.gz
rm mongo-c-driver-1.13.0.tar.gz
git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
git clone https://github.com/BVLC/caffe.git
svn co http://llvm.org/svn/llvm-project/test-suite/trunk suites/llvm-test-suite
git clone http://hera:8080/gerrit/wazuhl-clang tools/clang
docker-compose build