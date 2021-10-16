#!/bin/bash

if [ "`uname`" == "Linux" ]; then
    # Add repositories
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo add-apt-repository -y \
        "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-${LLVM_VERSION} main"
    sudo apt-get update -qq

    # Install Clang + LLVM
    sudo apt-get install -y \
        llvm-${LLVM_VERSION}-dev \
        libclang-${LLVM_VERSION}-dev \
        clang-${LLVM_VERSION} \
        libomp-${LLVM_VERSION}-dev
    sudo update-alternatives --install \
        /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
    sudo update-alternatives --install \
        /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20

    # Other dependencies
    sudo apt-get install -y libedit-dev
elif [ "`uname`" == "Darwin" ]; then
    URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}.0.0"
    ARCHIVE="clang+llvm-${LLVM_VERSION}.0.0-x86_64-apple-darwin.tar.xz"

    if [ ${LLVM_VERSION} -lt 13 ]; then
        ln -sfn /Applications/Xcode_12.4.app /Applications/Xcode.app
    fi

    mkdir -p llvm-${LLVM_VERSION}
    wget "$URL/$ARCHIVE"
    tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}
elif [[ "`uname`" == "MINGW64"* ]]; then
    if [ ! -r llvm-${LLVM_VERSION}/install/lib/cmake/llvm/LLVMConfig.cmake ]; then
        URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}.0.0"

        # Get LLVM
        ARCHIVE="llvm-${LLVM_VERSION}.0.0.src.tar.xz"
        mkdir -p llvm-${LLVM_VERSION}
        curl -OL "$URL/$ARCHIVE"
        tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}

        # Get Clang
        ARCHIVE="clang-${LLVM_VERSION}.0.0.src.tar.xz"
        mkdir -p llvm-${LLVM_VERSION}/tools/clang
        curl -OL "$URL/$ARCHIVE"
        tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}/tools/clang

        # Build LLVM + Clang
        mkdir -p llvm-${LLVM_VERSION}/build
        cd llvm-${LLVM_VERSION}/build
        cmake .. \
        -G "Visual Studio 16 2019" -A ${BUILD_PLATFORM} \
        -DCMAKE_INSTALL_PREFIX=$PWD/../install \
        -DLLVM_TARGETS_TO_BUILD=host
        cmake --build . --config Release --target ALL_BUILD
        cmake --build . --config Release --target INSTALL
    fi
else
  echo "Unrecognized uname: `uname`"
  exit 1
fi
