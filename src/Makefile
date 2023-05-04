.PHONY : all clean

all: miniGo

CFLAGS = -std=c++20 -I.
# show bison parsing trace
DEBUGFLAG = -DYYDEBUG

miniGo:
	@echo "--- Build Compiler ---"
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp $(CFLAGS)

ll: miniGo
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm runtime.c -o runtime.o.ll
	@echo "--- Build Main LL ---"
	./miniGo.out ../debug/main.go -o ../debug/main.o.ll

debugLL: 
	@echo "--- Build Compiler ---"
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp $(CFLAGS) $(DEBUGFLAG)
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm runtime.c -o runtime.o.ll
	@echo "--- Build Main LL ---"
	./miniGo.out ../debug/main.go -o ../debug/main.o.ll

test: ll
	@echo "--- Build Main ---"
	clang runtime.o.ll ../debug/main.o.ll -o ../debug/main.out
	@echo "--- Start Main ---"	
	@../debug/main.out

clean:
	@echo "--- Clean ---"	
	-rm -f *.yy.*
	-rm -f *.tab.*
	-rm -f *.o.ll
	-rm -f *.out
	-rm -f ../debug/*.o.ll
	-rm -f ../debug/*.out
