INSTALL_DIR="~/.oclgrind"
OS="linux-gnu-ubuntu-20.04"
#OS="apple-darwin"

LLVM_VERSION=13

#install llvm
URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}.0.0"
ARCHIVE="clang+llvm-${LLVM_VERSION}.0.0-x86_64-${OS}.tar.xz"
mkdir -p llvm-${LLVM_VERSION}
wget "$URL/$ARCHIVE"
tar xf "$ARCHIVE" --strip-components 1 -C llvm-${LLVM_VERSION}

#install Oclgrind
PROJ_DIR=`pwd`
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo\
         -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
         -DLLVM_ROOT_DIR="${PROJ_DIR}/llvm-${LLVM_VERSION}"
make install
