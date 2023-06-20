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
        libpolly-${LLVM_VERSION}-dev \
        libomp-${LLVM_VERSION}-dev
    sudo update-alternatives --install \
        /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
    sudo update-alternatives --install \
        /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20

    # Other dependencies
    sudo apt-get install -y libedit-dev libvulkan-dev
elif [ "`uname`" == "Darwin" ]; then
    URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}.0.0"
    ARCHIVE="clang+llvm-${LLVM_VERSION}.0.0-x86_64-apple-darwin.tar.xz"

    #hopefully temporary dirty hack to resolve bad pathing: No rule to make target `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.3.sdk/usr/lib/libcurses.tbd', needed by `liboclgrind.dylib'
    ln -s /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.3.sdk
    ln -s /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.1.sdk

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

        if [ ${LLVM_VERSION} == 14 ]; then
            mkdir -p cmake
            mv "llvm-${LLVM_VERSION}/Modules" cmake
        fi

        # Build LLVM + Clang
        mkdir -p llvm-${LLVM_VERSION}/build
        cd llvm-${LLVM_VERSION}/build
        cmake .. \
            -G "Visual Studio 16 2019" -A ${BUILD_PLATFORM} \
            -DCMAKE_INSTALL_PREFIX=$PWD/../install \
            -DLLVM_TARGETS_TO_BUILD=host \
            -DLLVM_INCLUDE_BENCHMARKS=OFF
        cmake --build . --config Release --target ALL_BUILD
        cmake --build . --config Release --target INSTALL
    fi
else
  echo "Unrecognized uname: `uname`"
  exit 1
fi
