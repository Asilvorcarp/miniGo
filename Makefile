# note:
# ensure debug/main.go for "ll"
# ensure debug/test.temp.in for "in"

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
	bison -t src/miniGo.y -o build/miniGo.tab.hpp

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

build/main.o.ll: debug/main.go build # TODO maybe change to main.ll
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

.PHONY: silent
silent: CFLAGS+=$(SILENTFLAG) build
silent: build

.PHONY: ll
ll: build/main.o.ll

.PHONY: debugLL
debugLL: CFLAGS+=$(DEBUGFLAG)
debugLL: ll

.PHONY: gdb
gdb: build
	gdb --args build/miniGo debug/main.go -o build/main.o.ll

# from .ll to .out
build/%.out: build/%.o.ll # TODO maybe change to .mini.out
	clang $< -o $@ -mllvm -opaque-pointers

.PHONY: main
main: build/main.out

# simple test by running main
.PHONY: test
test: main
	@echo "--- Run Main ---"	
	@build/main.out

# simple test by running main with input
.PHONY: in
in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in build/main.out

.PHONY: diffMain
diffMain : in
	cat debug/test.temp.in go run ./debug/main.go ./debug/Runtime.go > right.o.txt
	cat debug/test.temp.in build/main.out > my.o.txt

TEST_GO_SRCS=$(filter-out tests/Runtime.go, $(wildcard tests/*.go))
EXE_OUTS=$(patsubst tests/%.go, build/%.out, $(TEST_GO_SRCS))
RIGHT_EXE_OUTS=$(patsubst tests/%.go, build/%.right.out, $(TEST_GO_SRCS))

.PHONY: tests
tests: $(EXE_OUTS)
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.out < $$input_file > $$output_file |exit 1; \
				if [ $(SHOW_TEST_OUTPUT) -eq 1 ]; then \
					echo " - Output:"; \
					cat $$output_file; \
				fi; \
			done; \
		fi \
	done

# now use go official compiler for comparison

build/%.right.out: tests/%.go tests/Runtime.go
	go build -o $@ $< tests/Runtime.go

.PHONY: go_build
go_build: $(RIGHT_EXE_OUTS)

# get right.out # TODO maybe change name (two right now)
.PHONY: go_tests
go_tests: go_build
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.right.out; \
				build/$$base_name.right.out < $$input_file > $$output_file |exit 1; \
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
			for expected_file in tests/$$base_name/*.right.out; do \
				output_file=$${expected_file%.right.out}.out; \
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
time:
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
				if [ $(SHOW_TEST_OUTPUT) -eq 1 ]; then \
					echo " - Output:"; \
					cat $$output_file; \
				fi; \
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

.PHONY: clean
clean:
	@echo "--- Clean ---"
	rm -rf build/*
	find . -name "*.o.*" -delete
	find . -name "*.out" -delete
	find . -name "*.tab.*" -delete
	find . -name "*.yy.*" -delete
