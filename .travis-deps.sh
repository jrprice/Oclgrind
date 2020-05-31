#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "linux" ]
then
    # Add repositories
    sudo add-apt-repository -y \
        "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-${LLVM_VERSION} main"
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo apt-get update -qq

    # Remove existing LLVM
    sudo apt-get remove llvm
    sudo apt-get autoremove

    # Install Clang + LLVM
    sudo apt-get install -y \
        llvm-${LLVM_VERSION}-dev \
        libclang-${LLVM_VERSION}-dev \
        clang-${LLVM_VERSION}
    sudo update-alternatives --install \
        /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
    sudo update-alternatives --install \
        /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20

    # Other dependencies
    sudo apt-get install -y libedit-dev
elif [ "$TRAVIS_OS_NAME" == "osx" ]
then
    if [ "$LLVM_VERSION" = "10" ]
    then
        URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}.0.0"
    else
        URL="http://releases.llvm.org/${LLVM_VERSION}.0.0"
    fi
    ARCHIVE="clang+llvm-${LLVM_VERSION}.0.0-x86_64-apple-darwin.tar.xz"

    mkdir -p llvm-${LLVM_VERSION}
    wget "$URL/$ARCHIVE"
    tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}
fi
