#! /bin/bash

if [ $# -eq 0 ]
then
    echo "Usage: $0 source1.c source2.cpp ... [-o output_name]"
    exit 1
fi

output_name="program.out"
source_files=()

while [[ $# -gt 0 ]]
do
    case $1 in
        -o)
            output_name="$2"
            shift 2
        ;;
        *)
            source_files+=("$1")
            shift
        ;;
    esac
done

if [ -z "$FUEL" ]
then
    FUEL=10000
fi

object_files=()

if [[ "$OSTYPE" == "darwin"* ]]
then
    PLUGIN_EXT=".dylib"
    LINKER_FLAGS="-L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem"
else
    PLUGIN_EXT=".so"
    LINKER_FLAGS="-L/usr/lib -lc"
fi

export FUEL
for src in "${source_files[@]}"
do
    temp_obj=$(mktemp).o
    
    if [ ${#object_files[@]} -eq 0 ]
    then
        IS_FIRST_SRC=1
    else
        IS_FIRST_SRC=0
    fi
    
    ./build/bin/clang -O2 -fno-PIE -no-pie "$src" -emit-llvm -S -o - | \
    IS_FIRST_SRC=$IS_FIRST_SRC ./build/bin/opt \
        -load-pass-plugin ./build/lib/LLVMFuel$PLUGIN_EXT \
        -load-pass-plugin ./build/lib/LLVMRuntimeSig$PLUGIN_EXT \
        -passes="runtime-signature,fuel" -S -o - | \
    ./build/bin/llc -o - | \
    ./build/bin/clang -fno-PIE -no-pie -c -x assembler - -o "$temp_obj"
    
    if [ $? -ne 0 ]
    then
        echo "Failed to compile $src"
        exit 1
    fi
    
    object_files+=("$temp_obj")
done

echo ">> ${object_files[@]}"

./build/bin/clang -fno-PIE -no-pie "${object_files[@]}" -o "$output_name" $LINKER_FLAGS

if [ $? -eq 0 ]
then
    echo "Successfully compiled to $output_name"
    rm -f "${object_files[@]}"
else
    echo "Linking failed"
    exit 1
fi