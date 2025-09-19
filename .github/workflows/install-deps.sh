#!/bin/bash

LLVM_FULL_VERSION=${LLVM_VERSION}.1.0

if [ "`uname`" == "Linux" ]; then
    # Add repositories
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo add-apt-repository -y \
        "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-${LLVM_VERSION} main"
    sudo apt-get update -qq

    # Install Clang + LLVM
    sudo apt-get install -y \
        llvm-${LLVM_VERSION}-dev \
        libclang-${LLVM_VERSION}-dev \
        clang-${LLVM_VERSION} \
        libomp-${LLVM_VERSION}-dev \
        libpolly-${LLVM_VERSION}-dev
    sudo update-alternatives --install \
        /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
    sudo update-alternatives --install \
        /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20

    # Other dependencies
    sudo apt-get install -y libedit-dev libvulkan-dev
elif [ "`uname`" == "Darwin" ]; then
    brew install llvm@${LLVM_VERSION}
elif [[ "`uname`" == "MINGW64"* ]]; then
    if [ ! -r llvm-${LLVM_VERSION}/install/lib/cmake/llvm/LLVMConfig.cmake ]; then
        URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_FULL_VERSION}"

        # Get LLVM
        ARCHIVE="llvm-${LLVM_FULL_VERSION}.src.tar.xz"
        mkdir -p llvm-${LLVM_VERSION}/llvm
        curl -OL "$URL/$ARCHIVE"
        tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}/llvm

        # Get Clang
        ARCHIVE="clang-${LLVM_FULL_VERSION}.src.tar.xz"
        mkdir -p llvm-${LLVM_VERSION}/clang
        curl -OL "$URL/$ARCHIVE"
        tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}/clang

        # Get CMake helpers
        mkdir -p llvm-${LLVM_VERSION}/cmake
        ARCHIVE="cmake-${LLVM_FULL_VERSION}.src.tar.xz"
        curl -OL "$URL/$ARCHIVE"
        tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}/cmake

        # Build LLVM + Clang
        mkdir -p llvm-${LLVM_VERSION}/build
        cd llvm-${LLVM_VERSION}/build
        cmake ../llvm \
            -G "Visual Studio 17 2022" -A ${BUILD_PLATFORM} \
            -DCMAKE_INSTALL_PREFIX=$PWD/../install \
            -DLLVM_ENABLE_PROJECTS='clang' \
            -DLLVM_TARGETS_TO_BUILD=host \
            -DLLVM_INCLUDE_BENCHMARKS=OFF \
            -DLLVM_INCLUDE_TESTS=OFF
        cmake --build . --config Release --target ALL_BUILD
        cmake --build . --config Release --target INSTALL
    fi
else
  echo "Unrecognized uname: `uname`"
  exit 1
fi
