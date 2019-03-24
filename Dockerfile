FROM ubuntu:18.04

# Clone the following in this folder before start:
# wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz
# tar xzf mongo-c-driver-1.13.0.tar.gz
# rm mongo-c-driver-1.13.0.tar.gz
# git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
# git clone https://github.com/BVLC/caffe.git
# svn co http://llvm.org/svn/llvm-project/test-suite/trunk suites/llvm-test-suite
# git clone http://hera:8080/gerrit/wazuhl-clang tools/clang

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

VOLUME /wazuhl
VOLUME /wazuhl-build
VOLUME /suites

ADD libtorch /libtorch
ADD mongo-c-driver-1.13.0 /mongo-c-driver-1.13.0
ADD mongo-cxx-driver /mongo-cxx-driver

RUN apt-get update && apt-get install -y python3
RUN apt-get update && apt-get install -y python3-pip

# Caffe dependencies.
RUN apt-get update && apt-get install -y build-essential cmake git pkg-config libprotobuf-dev libleveldb-dev libsnappy-dev libhdf5-serial-dev protobuf-compiler libatlas-base-dev
RUN apt-get update && apt-get install -y --no-install-recommends libboost-all-dev
RUN apt-get update && apt-get install -y libgflags-dev libgoogle-glog-dev liblmdb-dev
RUN apt-get update && apt-get install -y python3-dev
RUN apt-get update && apt-get install -y python3-numpy python3-scipy
RUN apt-get install -y libopencv* opencv*
RUN apt-get update && apt-get install -y python-opencv

ENV LD_LIBRARY_PATH="/usr/local/lib/:/libtorch/lib/:"

# Mongo.
RUN apt-get update && apt-get install -y cmake libssl-dev libsasl2-dev
WORKDIR /mongo-c-driver-1.13.0
RUN mkdir cmake-build
WORKDIR /mongo-c-driver-1.13.0/cmake-build
RUN CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
RUN make
RUN make install

WORKDIR /mongo-cxx-driver/build
RUN CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
RUN make EP_mnmlstc_core
RUN make
RUN make install

# Wazuhl install.
RUN apt-get update && apt-get install -y ninja-build
RUN apt-get update && apt-get install -y vim

CMD bash /wazuhl/entrypoint.sh
