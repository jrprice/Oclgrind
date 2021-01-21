FROM ubuntu:xenial

#ENV https_proxy http://proxy.ftpn.ornl.gov:3128
#ENV http_proxy http://proxy.ftpn.ornl.gov:3128

RUN apt-get -yq update

# Utilities
RUN apt-get install -yq --allow-downgrades --allow-remove-essential            \
    --allow-change-held-packages git wget vim python-pip apt-utils cmake unzip                \
    libboost-all-dev software-properties-common python-software-properties libcompute-dev libreadline-dev autoconf

# Install LLVM-11.0.1 and clang---needed for AIWC
WORKDIR /
RUN wget -qO- "https://cmake.org/files/v3.12/cmake-3.12.1-Linux-x86_64.tar.gz" | tar --strip-components=1 -xz -C /usr
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/llvm-11.0.1.src.tar.xz
RUN tar -xvf llvm-11.0.1.src.tar.xz
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/clang-11.0.1.src.tar.xz
RUN tar -xvf clang-11.0.1.src.tar.xz && mv clang-11.0.1.src clang
#build a shared library version
WORKDIR /llvm-11.0.1.src/build
RUN cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_ASM_COMPILER=gcc -DBUILD_SHARED_LIBS=On -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/llvm-11.0.1 ..
RUN make -j8 install
RUN make -j8 clang install

# Architecture-Independent Workload Characterization (AIWC)
RUN mkdir /oclgrind
WORKDIR /oclgrind
RUN git clone https://github.com/BeauJoh/AIWC.git src
RUN mkdir /oclgrind/build
WORKDIR /oclgrind/build
ENV CC /llvm-11.0.1/bin/clang
ENV CXX /llvm-11.0.1/bin/clang++
RUN cmake /oclgrind/src -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_DIR=/llvm-11.0.1/lib/cmake/llvm -DCLANG_ROOT=/llvm-11.0.1 -DCMAKE_INSTALL_PREFIX=/oclgrind -DBUILD_SHARED_LIBS=On
RUN make
RUN make install
RUN mkdir -p /etc/OpenCL/vendors && echo /oclgrind/lib/liboclgrind-rt-icd.so > /etc/OpenCL/vendors/oclgrind.icd

# Install Extended Open Dwarfs Benchmark suite to test OpenCL and AIWC...
WORKDIR /ExtendedOpenDwarfs
RUN git clone https://github.com/BeauJoh/OpenDwarfs.git src
WORKDIR /ExtendedOpenDwarfs/src
RUN ./autogen.sh
WORKDIR /ExtendedOpenDwarfs/build
RUN ../src/configure
RUN make lud && cp ../src/dense-linear-algebra/lud/lud_kernel.cl .

#and run a test with an OpenCL version of LUD to generate AIWC features.
CMD OCLGRIND_WORKLOAD_CHARACTERISATION=1 OCLGRIND_WORKLOAD_CHARACTERISATION_OUTPUT_PATH=~/aiwc_metrics.csv ./lud -s 16

