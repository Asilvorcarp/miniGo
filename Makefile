.PHONY : all clean

all: miniGo

CFLAGS = -std=c++20 -Isrc -Ibuild
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG

# ensure debug/main.go for ll
# ensure debug/test.temp.in for test

miniGo:
	@echo "--- Build Compiler ---"
	@mkdir -p build
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS)

ll: miniGo
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o debug/main.o.ll

debugLL: 
	@echo "--- Build Compiler ---"
	flex -o build/miniGo.yy.cpp src/miniGo.l
	bison -t src/miniGo.y -o build/miniGo.tab.hpp
	clang++ -o build/miniGo.out build/miniGo.yy.cpp src/main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm src/runtime.c -o build/runtime.o.ll
	@echo "--- Build Main LL ---"
	build/miniGo.out debug/main.go -o debug/main.o.ll

main: ll
	@echo "--- Build Main ---"
	clang build/runtime.o.ll debug/main.o.ll -o build/main.out

test: main
	@echo "--- Run Main ---"	
	@build/main.out

in: main
	@echo "--- Run Main with Input ---"	
	@cat debug/test.temp.in
	@echo "--- Output ---"	
	@cat debug/test.temp.in | build/main.out

clean:
	@echo "--- Clean ---"	
	-rm -f **/*.yy.*
	-rm -f **/*.tab.*
	-rm -f **/*.o.*
	-rm -f **/*.out
	-rm -f ast.o.json
