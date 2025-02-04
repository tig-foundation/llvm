#!/bin/bash
BUILD_JOBS=$(nproc) ./build-llvm.sh #&& cp -r build build.bkup && ./build-rustc.sh

rustup install nightly-2024-12-17
rustup +nightly-2024-12-17 component add rust-src 
rustup +nightly-2024-12-17 target add aarch64-unknown-linux-gnu
rustup +nightly-2024-12-17 target add x86_64-unknown-linux-gnu

#rustup +nightly component add rust-src --toolchain stable-aarch64-apple-darwin
