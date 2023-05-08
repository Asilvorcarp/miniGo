.PHONY : all yacc ll clean debug compareLL built_tests tests winCp go_tests diff in

all: build

CFLAGS = -std=c++20 -Isrc -Ibuild
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG -g -O0
# no output from compiler, including stdout and ast.o.json
SILENTFLAG = -DSILENT -O3

# ensure debug/main.go for ll
# ensure debug/test.temp.in for test

HPP_FILES = $(wildcard src/*.hpp)
CPP_FILES = $(wildcard src/*.cpp)

.PHONY: folder
folder:
	mkdir -p build

build/miniGo.yy.cpp: folder src/miniGo.l $(HPP_FILES) $(CPP_FILES)
	flex -o build/miniGo.yy.cpp src/miniGo.l

build/miniGo.tab.hpp: src/miniGo.y build/miniGo.yy.cpp $(HPP_FILES) $(CPP_FILES)
	bison -t src/miniGo.y -o build/miniGo.tab.hpp

.PHONY: yacc
yacc: build/miniGo.yy.cpp build/miniGo.tab.hpp

build/miniGo: src/main.cpp yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS)

.PHONY: build
build: build/miniGo

build/main.o.ll: build
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

.PHONY: ll
ll: build/main.o.ll

.PHONY: debugLL
debugLL: yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

.PHONY: gdb
gdb: build
	gdb --args build/miniGo debug/main.go -o build/main.o.ll

build/main.out: ll
	@echo "--- Build Main ---"
	clang build/main.o.ll -o build/main.out -mllvm -opaque-pointers

.PHONY: main
main: build/main.out

# TODO why test dont run ll
.PHONY: test
test: main
	@echo "--- Run Main ---"	
	@build/main.out

silent: yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(SILENTFLAG)

CLANG_LINK = clang build/$1.o.ll -o build/$1.out -mllvm -opaque-pointers

# build silent before tests

tests: silent
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			build/miniGo $$test_file -o build/$$base_name.o.ll; \
			$(call CLANG_LINK,$$base_name); \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.out < $$input_file > $$output_file |exit 1; \
				echo " - Output:"; \
				cat $$output_file; \
			done; \
		fi \
	done

test_files=$(filter-out src/Runtime.go, $(wildcard src/*.go))
go_exes=$(patsubst src/%.go, build/%.right.out, $(test_files))

build/%.right.out: src/%.go
	go build -o $@ $< tests/Runtime.go

go_build: $(go_exes)

INPUT=tests/course_selection/3.in
EXEC1=build/course_selection.right.out
EXEC2=build/course_selection.out

perf:
	@echo "Running $(EXEC1)..."
	@time $(EXEC1) < $(INPUT) > /dev/null
	@echo "Running $(EXEC2)..."
	@time $(EXEC2) < $(INPUT) > /dev/null

perf2:
	# @time $(EXEC1) < $(INPUT) > /dev/null
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			build/miniGo $$test_file -o build/$$base_name.o.ll; \
			$(call CLANG_LINK,$$base_name); \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.out < $$input_file > $$output_file |exit 1; \
				echo " - Output:"; \
				cat $$output_file; \
			done; \
		fi \
	done

go_tests:
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.right.out; \
				go run tests/Runtime.go $$test_file < $$input_file > $$output_file |exit 1; \
				echo " - Output:"; \
				cat $$output_file; \
			done; \
		fi \
	done

diff: tests go_tests
	@all_same=true; \
	for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > On test: $$base_name"; \
			for expected_file in tests/$$base_name/*.right.out; do \
				output_file=$${expected_file%.right.out}.out; \
				echo " - Diffing $$expected_file with $$output_file"; \
				diff --strip-trailing-cr $$expected_file $$output_file; \
				if [ $$? -eq 0 ]; then \
					echo " - Pass!"; \
				else \
					all_same=false; \
				fi \
			done; \
		fi \
	done; \
	if [ $$all_same = true ]; then \
		echo "Pass All Tests!"; \
	else \
		echo "There are differences between some output files and expected files!"; \
	fi

in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in build/main.out

compareResult : in
	cat debug/test.temp.in go run ./debug/main.go ./debug/Runtime.go > right.o.txt
	cat debug/test.temp.in build/main.out > my.o.txt

# Cross compile for windows x86_64 (needs to be statically linked)
# with package mingw-w64-gcc installed on archlinux

CROSSFLAGS = -static-libgcc -static-libstdc++

build/miniGo.exe: yacc
	@echo "--- Build Compiler ---"
	x86_64-w64-mingw32-g++ -o build/miniGo.exe build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(CROSSFLAGS)

win: build/miniGo.exe

# copy to windows and run

win-test: win
	cp -r tests/* /win/study/fc/project/go/
	cp build/miniGo.exe /win/study/fc/project/go/
	# call pwsh now!
	/mnt/d/Pros/PowerShell/7/pwsh.exe -c 'cd D:\Win\study\fc\project\go\ && ./test.ps1'

clean:
	@echo "--- Clean ---"
	rm -rf build/*
	find . -name "*.o.*" -delete
	find . -name "*.out" -delete
	find . -name "*.tab.*" -delete
	find . -name "*.yy.*" -delete
