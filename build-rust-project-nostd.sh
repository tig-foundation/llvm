#!/bin/bash

if [ $# -eq 0 ]
then
    echo "Usage: $0 project_dir [-o output_name]"
    exit 1
fi

output_name="program.out"
project_dir=""

while [[ $# -gt 0 ]]
do
    case $1 in
        -o)
            output_name="$2"
            shift 2
        ;;
        *)
            project_dir="$1"
            shift
        ;;
    esac
done

pushd "$project_dir"

if [[ "$OSTYPE" == "darwin"* ]]
then
    PLUGIN_EXT=".dylib"
    LINKER_FLAGS="-L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem"
    ENTRY_POINT="_main"
else
    PLUGIN_EXT=".so"
    LINKER_FLAGS="-static"
    ENTRY_POINT="main"
fi

TARGET=""
if [[ "$(uname -m)" == "arm64" ]] || [[ "$(uname -m)" == "aarch64" ]]
then
    TARGET="aarch64-unknown-linux-gnu"
else
    TARGET="x86_64-unknown-linux-gnu"
fi

if [ -z "$TARGET" ]
then
    echo "Failed to determine target architecture"
    exit 1
fi

export RUSTFLAGS="-C panic=abort --emit=llvm-ir -C embed-bitcode=yes -C codegen-units=1 -C prefer-dynamic=no -C lto=no -C debuginfo=2 -C link-args=-lc"

cargo +nightly-2024-12-17 build \
    --target=$TARGET \
    -Z build-std=core,alloc,compiler_builtins \
    -Z build-std-features=panic_immediate_abort \
    -v

popd

if [ $? -ne 0 ]
then
    echo "Failed to compile Rust project"
    exit 1
fi

if [ -z "$FUEL" ]
then
    FUEL=10000
fi

export FUEL

ll_files=()
while IFS= read -r line; do
    ll_files+=("$line")
done < <(find "$project_dir/target/$TARGET/debug/deps" -name "*.ll")

echo "LL files: ${ll_files[@]}"

object_files=()

for ll_file in "${ll_files[@]}"
do
    temp_obj=$(mktemp).o

    if [ ${#object_files[@]} -eq 0 ]
    then
        IS_FIRST_SRC=1
    else
        IS_FIRST_SRC=0
    fi

    cat "$ll_file" | \
    IS_FIRST_SRC=$IS_FIRST_SRC ./build/bin/opt \
        -load-pass-plugin ./build/lib/LLVMFuel$PLUGIN_EXT \
        -load-pass-plugin ./build/lib/LLVMRuntimeSig$PLUGIN_EXT \
        -passes="runtime-signature,fuel" -S -o - | \
    ./build/bin/llc -o - | \
    ./build/bin/clang -fno-PIC -c -x assembler - -o "$temp_obj"

    if [ $? -ne 0 ]
    then
        echo "Failed to process $ll_file"
        exit 1
    fi

    object_files+=("$temp_obj")
done

./build/bin/clang "${object_files[@]}" \
    -o "$output_name" \
    -static \
    -L/usr/lib/aarch64-linux-gnu \
    -L/usr/lib/gcc/aarch64-linux-gnu/12 \
    -L/root/.rustup/toolchains/nightly-2024-12-17-$TARGET/lib/rustlib/$TARGET/lib \
    -Wl,--start-group \
    -lc \
    -lgcc \
    -lgcc_eh \
    -lm \
    -Wl,--end-group \
    -Wl,--gc-sections \
    -Wl,-z,noexecstack \
    -Wl,--build-id=none \
    -Wl,--eh-frame-hdr \
    -Wl,-Map=output.map

if [ $? -eq 0 ]
then
    echo "Successfully compiled to $output_name"
    rm -f "${object_files[@]}"
else
    echo "Linking failed"
    exit 1
fi
