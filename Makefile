.PHONY : all clean debug compareLL built_tests tests winCp go_tests diff in

all: build

CFLAGS = -std=c++20 -Isrc -Ibuild
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG -g -O0
# no output from compiler, including stdout and ast.o.json
SILENTFLAG = -DSILENT -O3

# ensure debug/main.go for ll
# ensure debug/test.temp.in for test

yacc:
	@mkdir -p build
	@echo "--- Build Yacc ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp

build: yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS)

ll: build
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

debugLL: yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Main LL ---"
	build/miniGo debug/main.go -o build/main.o.ll

gdb:
	gdb --args build/miniGo debug/main.go -o build/main.o.ll

main: ll
	@echo "--- Build Main ---"
	clang build/main.o.ll -o build/main.out

test: main
	@echo "--- Run Main ---"	
	@build/main.out

silent: yacc
	@echo "--- Build Compiler ---"
	clang++ -o build/miniGo build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(SILENTFLAG)

CLANG_LINK = clang build/$1.o.ll -o build/$1.out

# build silent before tests

built_tests:
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			build/miniGo $$test_file -o build/$$base_name.o.ll; \
			$(call CLANG_LINK,$$base_name); \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.out; \
				./build/$$base_name.out < $$input_file > $$output_file || exit 1; \
				echo " - Output:"; \
				cat $$output_file; \
			done; \
		fi \
	done

tests: silent built_tests

go_tests:
	@for test_file in tests/*.go; do \
		base_name=$$(basename $$test_file .go); \
		if [ $$base_name != "Runtime" ]; then \
			echo " > Running test: $$base_name"; \
			for input_file in tests/$$base_name/*.in; do \
				echo " - Input file: $$input_file"; \
				output_file=$${input_file%.in}.right.out; \
				go run tests/Runtime.go $$test_file < $$input_file > $$output_file || exit 1; \
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
	@cat debug/test.temp.in | build/main.out

compareResult : in
	cat debug/test.temp.in | go run ./debug/main.go ./debug/Runtime.go > right.o.txt
	cat debug/test.temp.in | build/main.out > my.o.txt

# Cross compile for windows x86_64 (needs to be statically linked)
# with package mingw-w64-gcc installed on archlinux

CROSSFLAGS = -static-libgcc -static-libstdc++

win: yacc
	@echo "--- Build Compiler ---"
	x86_64-w64-mingw32-g++ -o build/miniGo.exe build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(CROSSFLAGS)

# copy to windows and run

win-test: win
	cp -r tests/* /win/study/fc/project/go/
	cp build/miniGo.exe /win/study/fc/project/go/
	# call pwsh now!
	/mnt/d/Pros/PowerShell/7/pwsh.exe -c 'cd D:\Win\study\fc\project\go\ && ./test.ps1'

clean:
	@echo "--- Clean ---"
	-rm -f build/*.exe
	-rm -f build/miniGo
	-rm -f ast.o.json
	-find . -name "*.o.*" | xargs rm -f
	-find . -name "*.out" | xargs rm -f
	-find . -name "*.tab.*" | xargs rm -f
	-find . -name "*.yy.*" | xargs rm -f
