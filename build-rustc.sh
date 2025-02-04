#!/bin/bash

if !command -v rustup >/dev/null 2>&1
then
    echo "rustup not found. Please install rustup first: https://rustup.rs/"
    exit 1
fi


if command -v apt >/dev/null 2>&1
then
    sudo apt install -y make cmake gcc clang build-essential python3 gcc-aarch64-linux-gnu g++-aarch64-linux-gnu ninja-build libzstd-dev
fi

if command -v brew >/dev/null 2>&1
then
    brew install cmake python3 aarch64-elf-gcc aarch64-elf-binutils ninja zstd

    # TOOLCHAIN_VERSION="12.3.rel1"
    # TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu/$TOOLCHAIN_VERSION/binrel/arm-gnu-toolchain-$TOOLCHAIN_VERSION-darwin-arm64-aarch64-none-linux-gnu.tar.xz"
    # TOOLCHAIN_DIR="$HOME/.local/arm-gnu-toolchain"
    
    # if [ ! -d "$TOOLCHAIN_DIR" ]; then
    #     echo "Downloading ARM GNU toolchain..."
    #     curl -L "$TOOLCHAIN_URL" -o arm-toolchain.tar.xz
        
    #     echo "Extracting toolchain..."
    #     mkdir -p "$TOOLCHAIN_DIR"
    #     tar xf arm-toolchain.tar.xz -C "$TOOLCHAIN_DIR" --strip-components=1
    #     rm arm-toolchain.tar.xz
    # fi

    # export PATH="$TOOLCHAIN_DIR/bin:$PATH"
    # export LIBRARY_PATH="$TOOLCHAIN_DIR/lib:$LIBRARY_PATH"
    # export LD_LIBRARY_PATH="$TOOLCHAIN_DIR/lib:$LD_LIBRARY_PATH"
    # export CPATH="$TOOLCHAIN_DIR/include:$CPATH"

    # BREW_PREFIX=$(brew --prefix)
    # LINK_PATH="$HOME/.local/bin"
    # mkdir -p "$LINK_PATH"

    # for tool in gcc g++ ar ranlib nm ld
    # do
    #     rm -f "$LINK_PATH/aarch64-linux-gnu-$tool"
    #     ln -sf "$TOOLCHAIN_DIR/bin/aarch64-none-linux-gnu-$tool" "$LINK_PATH/aarch64-linux-gnu-$tool"
    # done

    # export CC_aarch64_unknown_linux_gnu=aarch64-linux-gnu-gcc
    # export CXX_aarch64_unknown_linux_gnu=aarch64-linux-gnu-g++
    # export AR_aarch64_unknown_linux_gnu=aarch64-linux-gnu-ar
    # export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=aarch64-linux-gnu-gcc

    BREW_PREFIX=$(brew --prefix)
    export PATH="$BREW_PREFIX/opt/zstd/bin:$PATH"
    export LIBRARY_PATH="$BREW_PREFIX/opt/zstd/lib:$LIBRARY_PATH"
fi

rm -rf ./rust/src/llvm-project
mkdir -p ./rust/src/llvm-project
for file in *
do
    if [ "$file" != "build" ] && [ "$file" != "build-llvm" ] && [ "$file" != "rust" ] && \
        && [ "$file" != "build-rustc.sh" ] && [ "$file" != "setup.sh" ] && [ "$file" != "build-rust-project.sh" ] \
        && [ "$file" != "build-llvm.sh" ] && [ "$file" != "sample_rust" ]
    then
        ln -sf "$(pwd)/$file" "./rust/src/llvm-project/$file"
    fi
done

CURR_DIR=$(pwd)

ulimit -n 65535

pushd rust

if [[ "$OSTYPE" == "darwin"* ]]; then
    TARGET="aarch64-apple-darwin"
else
    TARGET="aarch64-unknown-linux-gnu"
fi

python3 src/bootstrap/configure.py \
    --llvm-config="$CURR_DIR/build/bin/llvm-config" \
    --llvm-filecheck="$CURR_DIR/build/bin/FileCheck" \
    --enable-llvm-plugins \
    --enable-optimize-llvm \
    --enable-llvm-assertions \
    --enable-debug-assertions \
    --enable-debug-assertions-std \
    --enable-overflow-checks \
    --enable-overflow-checks-std \
    --enable-verbose-tests \
    --enable-codegen-tests \
    --enable-locked-deps \
    --enable-extended \
    --enable-lld \
    --enable-llvm-bitcode-linker \
    --enable-clang \
    --enable-full-tools \
    --bindir=../rust_build/bin \
    --libdir=../rust_build/lib \
    --datadir=../rust_build/share \
    --sysconfdir=../rust_build/etc \
    --prefix=../rust_build \
    --mandir=../rust_build/share/man \
    --docdir=../rust_build/share/doc \
    --target=$TARGET \
    --set rust.lto=thin \
    --set rust.download-rustc=false \
    --set llvm.download-ci-llvm=if-unchanged \
    --set rust.jemalloc=true

./x.py build --stage 2 compiler/rustc && ./x.py install && ./x.py dist

#rustup toolchain link tig-tc build/aarch64-apple-darwin/stage2
if [[ "$OSTYPE" == "darwin"* ]]
then
    tar -xf build/dist/rust-nightly-aarch64-apple-darwin.tar.gz
    tar -xf build/dist/rust-src-nightly.tar.gz
    
    rustup toolchain link tig-tc "$(pwd)/rust-nightly-aarch64-apple-darwin/rustc"
    
    mkdir -p "$(pwd)/rust-nightly-aarch64-apple-darwin/rustc/lib/rustlib/src/rust"
    cp -r ./rust-src-nightly/rust-src/lib/rustlib/src/rust/library "$(pwd)/rust-nightly-aarch64-apple-darwin/rustc/lib/rustlib/src/rust/"
    
    rustup target add aarch64-apple-darwin --toolchain tig-tc
else
    tar -xf build/dist/rust-nightly-aarch64-unknown-linux-gnu.tar.gz
    tar -xf build/dist/rust-src-nightly.tar.gz
    
    rustup toolchain link tig-tc "$(pwd)/rust-nightly-aarch64-unknown-linux-gnu/rustc"
    
    mkdir -p "$(pwd)/rust-nightly-aarch64-unknown-linux-gnu/rustc/lib/rustlib/src/rust"
    cp -r ./rust-src-nightly/rust-src/lib/rustlib/src/rust/library "$(pwd)/rust-nightly-aarch64-unknown-linux-gnu/rustc/lib/rustlib/src/rust/"
    
    rustup target add aarch64-unknown-linux-gnu --toolchain tig-tc
fi

popd
