#!/bin/bash

if command -v apt >/dev/null 2>&1
then
    sudo apt install -y make cmake gcc clang build-essential
fi

if command -v brew >/dev/null 2>&1
then
    brew install cmake
fi

ulimit -n 65535

if [ -z "$BUILD_JOBS" ]
then
    BUILD_JOBS=$(nproc)
fi

CC=$(which clang) CXX=$(which clang++) cmake -S llvm -B build-llvm \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS=clang \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_TARGETS_TO_BUILD="AArch64;X86" &&
CC=$(which clang) CXX=$(which clang++) cmake --build build-llvm -j$BUILD_JOBS &&
cmake -DCMAKE_INSTALL_PREFIX=./build/ -P build-llvm/cmake_install.cmake

if [ $? -ne 0 ]; then
    echo "Failed to build LLVM"
    exit 1
fi

# build libmalloc.so
PATH="$(pwd)/build/bin:$PATH" clang -o build/lib/libmalloc.o snapshot/malloc.c -O3 -c
if [ $? -ne 0 ]; then
    echo "Failed to build libmalloc.o"
    exit 1
fi

echo "Successfully built libmalloc.o"