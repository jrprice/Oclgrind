#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "linux" ]
then
    # Add repositories
    sudo add-apt-repository -y 'deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main'
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo apt-get update -qq

    # Remove existing LLVM
    sudo apt-get remove llvm

    # Install Clang + LLVM
    sudo apt-get install -y llvm-3.9-dev libclang-3.9-dev clang-3.9
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.9 20
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.9 20
    sudo rm -f /usr/local/clang-3.5.0/bin/clang
    sudo rm -f /usr/local/clang-3.5.0/bin/clang++

    # Other dependencies
    sudo apt-get install -y libedit-dev
elif [ "$TRAVIS_OS_NAME" == "osx" ]
then
    brew update
    brew install -v llvm --with-clang
fi
