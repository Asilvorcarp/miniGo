.PHONY : all clean debug compareLL tests

all: build

CFLAGS = -std=c++20 -Isrc -Ibuild
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG -g -O0
# no output from compiler, including stdout and ast.o.json
SILENTFLAG = -DSILENT -O3

# ensure debug/main.go for ll
# ensure debug/test.temp.in for test

build:
	@echo "--- Build Compiler ---"
	@mkdir -p build
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS)

ll: build
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o build/main.o.ll

debugLL: 
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o build/main.o.ll

gdb:
	gdb --args build/miniGo.out debug/main.go -o build/main.o.ll

main: ll
	@echo "--- Build Main ---"
	clang build/runtime.o.ll build/main.o.ll -o build/main.out

test: main
	@echo "--- Run Main ---"	
	@build/main.out

silent:
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(SILENTFLAG)

BUILD_RUNTIME = clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
CLANG_LINK = clang build/runtime.o.ll build/$1.o.ll -o build/$1.out

# build silent before tests

tests:
	@for test_file in tests/*.go; do \
        base_name=$$(basename $$test_file .go); \
        echo "Running test: $$base_name"; \
        $(BUILD_RUNTIME); \
        build/miniGo.out $$test_file -o build/$$base_name.o.ll; \
        $(call CLANG_LINK,$$base_name); \
        for input_file in tests/$$base_name.*in; do \
            echo "Input file: $$input_file"; \
            echo "Output:"; \
            ./build/$$base_name.out < $$input_file || exit 1; \
        done; \
    done

in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in | build/main.out

compareLL : in
	cat debug/test.temp.in | go run ./debug/main.go ./debug/runtime.go > right.o.txt
	cat debug/test.temp.in | build/main.out > my.o.txt

clean:
	@echo "--- Clean ---"	
	-rm -f **/*.yy.*
	-rm -f **/*.tab.*
	-rm -f **/*.o.*
	-rm -f **/*.out
	-rm -f ast.o.json
