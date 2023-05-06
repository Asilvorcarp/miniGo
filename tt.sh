#!/bin/bash

for test_file in tests/*.go; do
    base_name=$(basename "$test_file" .go)
    echo " > Running test: $base_name"

    # get base.out
	clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
	build/miniGo.out tests/"$base_name".go -o build/"$base_name".o.ll
	clang build/runtime.o.ll build/$base_name.o.ll -o build/$base_name.out

    # foreach tests/X.num.in as input_file
    for input_file in tests/"$base_name".*in; do
        echo " - Input file: $input_file"
        echo " - Output:"
        ./"build/$base_name.out" < "$input_file" || exit 1
    done
done
