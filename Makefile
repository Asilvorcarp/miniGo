# note:
# ensure debug/main.go for "ll"
# ensure debug/test.temp.in for "in"

# extensions:
# - .bin  : binary executable in Linux
# - .exe  : binary executable in Windows
# - .ll   : LLVM IR
# - .go   : source code in Golang
# - .out  : output text file
# - .in   : input text file
# - .Go.* : Go official compiler output
# - .s    : assembly code from my backend
# - .llc.s: assembly code from llc
# - .run  : executable file from my backend


.PHONY: all
all: build

CFLAGS = -std=c++20 -Isrc -Ibuild
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG -g -O0
# no output from compiler, including stdout and ast.o.json
SILENTFLAG = -DSILENT -O3
# flags of cross compile for windows
CROSSFLAGS = -static-libgcc -static-libstdc++
# whether to show test output in tests and go_tests
SHOW_TEST_OUTPUT = 1
# source files
HPP_FILES = $(wildcard src/*.hpp)
CPP_FILES = $(wildcard src/*.cpp)

.PHONY: folder
folder:
	mkdir -p build

build/miniGo.yy.cpp: folder src/miniGo.l $(HPP_FILES) $(CPP_FILES)
	flex -o build/miniGo.yy.cpp src/miniGo.l

build/miniGo.tab.hpp: src/miniGo.y build/miniGo.yy.cpp $(HPP_FILES) $(CPP_FILES)
	bison -t src/miniGo.y -o build/miniGo.tab.hpp 2> /dev/null

.PHONY: yacc
yacc: build/miniGo.yy.cpp build/miniGo.tab.hpp

build/miniGo: src/main.cpp yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS)

.PHONY: build
build: build/miniGo

# from .go to .ll
build/%.o.ll: tests/%.go build
	@echo "--- Build LL ---"
	build/miniGo $< -o $@
build/%.o.ll: debug/%.go build
	@echo "--- Build Debug LL ---"
	build/miniGo $< -o $@

build/main.o.ll: debug/main.go build
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

.PHONY: silent
silent: CFLAGS+=$(SILENTFLAG) build
silent: build

.PHONY: ll
ll: build/main.o.ll

.PHONY: gdb
gdb: build
	gdb --args build/miniGo debug/main.go -o build/main.o.ll

# TODO maybe change to .mini.out
# from .ll to .bin
# TODO add my backend
build/%.bin: build/%.o.ll 
	@echo "--- Build Bin ---"
	clang $< -o $@

.PHONY: main
main: build/main.bin

# simple test by running main
.PHONY: test
test: main
	@echo "--- Run Main ---"	
	@build/main.bin

# simple test by running main with input
.PHONY: in
in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in | build/main.bin

.PHONY: diffMain
diffMain : in
	cat debug/test.temp.in | go run ./debug/main.go ./debug/Runtime.go > Go.out
	cat debug/test.temp.in | build/main.bin > mini.out

GO_SRCS=$(filter-out tests/Runtime.go, $(wildcard tests/*.go))
MINI_BINS=$(patsubst tests/%.go, build/%.bin, $(GO_SRCS))
GO_BINS=$(patsubst tests/%.go, build/%.Go.bin, $(GO_SRCS))
LLS=$(patsubst tests/%.go, build/%.o.ll, $(GO_SRCS))
RUNS=$(patsubst tests/%.go, build/%.run, $(GO_SRCS))

.PHONY: mini_build
mini_build: $(MINI_BINS)

.PHONY: runs
runs: $(RUNS)

.PHONY: lls
lls: $(LLS)

.PHONY: tests
tests: mini_build
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.bin < $$input_file > $$output_file |exit 1; \
				if [ $(SHOW_TEST_OUTPUT) -eq 1 ]; then \
					echo " - Output:"; \
					cat $$output_file; \
				fi; \
			done; \
		fi \
	done

.PHONY: run_tests
run_tests: runs
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.run < $$input_file > $$output_file |exit 1; \
				if [ $(SHOW_TEST_OUTPUT) -eq 1 ]; then \
					echo " - Output:"; \
					cat $$output_file; \
				fi; \
			done; \
		fi \
	done

# now use go official compiler for comparison

build/%.Go.bin: tests/%.go tests/Runtime.go
	go build -o $@ $< tests/Runtime.go
build/%.Go.bin: debug/%.go debug/Runtime.go
	go build -o $@ $< debug/Runtime.go

.PHONY: go_build
go_build: $(GO_BINS)

.PHONY: go_run
go_run: build/$(A).Go.bin
	./build/$(A).Go.bin

# get Go.out # TODO maybe change name (two right now)
.PHONY: go_tests
go_tests: go_build
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.Go.out; \
				build/$$base_name.Go.bin < $$input_file > $$output_file |exit 1; \
				if [ $(SHOW_TEST_OUTPUT) -eq 1 ]; then \
					echo " - Output:"; \
					cat $$output_file; \
				fi; \
			done; \
		fi \
	done

.PHONY: diff
diff: SHOW_TEST_OUTPUT=0
diff: tests go_tests
	@all_same=true; \
	for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > On test: $$base_name"; \
			for expected_file in tests/$$base_name/*.Go.out; do \
				output_file=$${expected_file%.Go.out}.out; \
				echo -n " - Diffing $$expected_file with $$output_file:"; \
				diff --strip-trailing-cr $$expected_file $$output_file; \
				if [ $$? -eq 0 ]; then \
					echo " Pass."; \
				else \
					all_same=false; \
				fi \
			done; \
		fi \
	done; \
	if [ $$all_same = true ]; then \
		echo "All Tests Passed!"; \
	else \
		echo "Some of Tests Failed."; \
		exit 1; \
	fi

.PHONY: run_diff
run_diff: SHOW_TEST_OUTPUT=0
run_diff: run_tests go_tests
	@all_same=true; \
	for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > On test: $$base_name"; \
			for expected_file in tests/$$base_name/*.Go.out; do \
				output_file=$${expected_file%.Go.out}.out; \
				echo -n " - Diffing $$expected_file with $$output_file:"; \
				diff --strip-trailing-cr $$expected_file $$output_file; \
				if [ $$? -eq 0 ]; then \
					echo " Pass."; \
				else \
					all_same=false; \
				fi \
			done; \
		fi \
	done; \
	if [ $$all_same = true ]; then \
		echo "All Tests Passed!"; \
	else \
		echo "Some of Tests Failed."; \
		exit 1; \
	fi

# time tests on both go and miniGo generated executables
.PHONY: time
time: mini_build go_build
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				echo " - Time of build/$$base_name.bin"; \
				time build/$$base_name.bin < $$input_file > /dev/null; \
				echo " - Time of build/$$base_name.Go.bin"; \
				time build/$$base_name.Go.bin < $$input_file > /dev/null; \
			done; \
		fi \
	done

# Cross compile for windows x86_64 (needs to be statically linked)
# with package mingw-w64-gcc installed on archlinux

build/miniGo.exe: yacc
	@echo "--- Build Compiler ---"
	x86_64-w64-mingw32-g++ -o build/miniGo.exe build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(CROSSFLAGS)

.PHONY: win
win: build/miniGo.exe

# copy to windows and run
.PHONY: win-test
win-test: win
	cp -r tests/* /win/study/fc/project/go/
	cp build/miniGo.exe /win/study/fc/project/go/
	# call pwsh now!
	/mnt/d/Pros/PowerShell/7/pwsh.exe -c 'cd D:\Win\study\fc\project\go\ && ./test.ps1'

build/main.llc.s: ll
	llc -march=x86-64 -filetype=asm build/main.o.ll -o build/main.llc.s -O0
	./simplify.sh build/main.llc.s

build/%.llc.s: build/%.o.ll
	@echo "--- Build Debug ASM ---"
	llc -march=x86-64 -filetype=asm $< -o $@ -O0
	./simplify.sh $@

build/%.s: build/%.o.ll
	@echo "--- Build ASM with My Backend---"
	python src/Backend.py -f $< -o $@ > /dev/null

build/%.s: build/%.o.ll
	@echo "--- Build ASM with My Backend---"
	python src/Backend.py -f $< -o $@ > backend.temp.log

.PHONY: asm ll2asm asm2bin deASM
asm: build/main.llc.s
ll2asm:
	llc -march=x86-64 -filetype=asm build/main.o.ll -o build/main.llc.s -O0
	./simplify.sh build/main.llc.s
asm2bin:
	gcc -o build/main.llc.bin build/main.llc.s
deASM: build/$(A).llc.s

build/%.run: build/%.s
	@echo "--- Build Executable ---"
	gcc $< -o $@

.PHONY: getLL getS getRun run
getLL: build/$(A).o.ll
getS: build/$(A).s
getRun: build/$(A).run
run: getRun
	@echo "--- Run ---"
	./build/$(A).run

.PHONY: debugLL
debugLL: CFLAGS+=$(DEBUGFLAG)
debugLL: build/$(A).o.ll

.PHONY: clean
clean:
	@echo "--- Clean ---"
	rm -rf build/*
	find . -name "*.o.*" -delete
	find . -name "*.out" -delete
	find . -name "*.tab.*" -delete
	find . -name "*.yy.*" -delete
