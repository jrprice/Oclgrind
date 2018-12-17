#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "linux" ]
then
    # Add repositories
    sudo add-apt-repository -y 'deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-4.0 main'
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo apt-get update -qq

    # Remove existing LLVM
    sudo apt-get remove llvm

    # Install Clang + LLVM
    sudo apt-get install -y llvm-4.0-dev libclang-4.0-dev clang-4.0
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-4.0 20
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-4.0 20
    sudo rm -f /usr/local/clang-3.5.0/bin/clang
    sudo rm -f /usr/local/clang-3.5.0/bin/clang++

    # Other dependencies
    sudo apt-get install -y libedit-dev
elif [ "$TRAVIS_OS_NAME" == "osx" ]
then
    wget http://releases.llvm.org/7.0.0/clang+llvm-7.0.0-x86_64-apple-darwin.tar.xz
    mkdir -p llvm-7.0
    tar xf clang+llvm-7.0.0-x86_64-apple-darwin.tar.xz --strip-components 1 -C llvm-7.0
fi
