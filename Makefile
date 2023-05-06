.PHONY : all clean debug compareLL built_tests tests

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
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o build/main.o.ll

debugLL: 
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o build/main.o.ll

gdb:
	gdb --args build/miniGo.out debug/main.go -o build/main.o.ll

main: ll
	@echo "--- Build Main ---"
	clang build/main.o.ll -o build/main.out

test: main
	@echo "--- Run Main ---"	
	@build/main.out

silent:
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(SILENTFLAG)

CLANG_LINK = clang build/$1.o.ll -o build/$1.out

# build silent before tests

built_tests:
	@for test_file in tests/*.go; do \
        base_name=$$(basename $$test_file .go); \
        echo "Running test: $$base_name"; \
        build/miniGo.out $$test_file -o build/$$base_name.o.ll; \
        $(call CLANG_LINK,$$base_name); \
        for input_file in tests/$$base_name/*.in; do \
            echo "Input file: $$input_file"; \
            echo "Output:"; \
            ./build/$$base_name.out < $$input_file || exit 1; \
        done; \
    done

tests: silent built_tests

in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in | build/main.out

compareResult : in
	cat debug/test.temp.in | go run ./debug/main.go ./debug/_runtime.go > right.o.txt
	cat debug/test.temp.in | build/main.out > my.o.txt

# Cross compile for windows x86_64 (needs to be statically linked)
# with package mingw-w64-gcc installed on archlinux

CROSSFLAGS = -static-libgcc -static-libstdc++

win:
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	x86_64-w64-mingw32-g++ -o build/miniGo.exe build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(CROSSFLAGS)

clean:
	@echo "--- Clean ---"	
	-rm -f **/*.yy.*
	-rm -f **/*.tab.*
	-rm -f **/*.o.*
	-rm -f **/*.out
	-rm -f ast.o.json
