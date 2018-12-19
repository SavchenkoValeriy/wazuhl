# wazuhl-training-ground
This repository contains an infrastructure to train Wazuhl

# Running under Docker

    sudo docker-compose build
    sudo docker-compose up -d
    sudo docker run -it -v ~/llvm:/clang wazuhl-training-ground_polygon

 Where ~/llvm is the directory with clang.
 Note that llvm tests are not here. Check out in suites/llvm_test_suite from this dir:
 
    svn co http://llvm.org/svn/llvm-project/test-suite/trunk suites/llvm_test_suite