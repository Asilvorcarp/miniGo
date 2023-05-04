.PHONY : all clean

all: miniGo

debug:
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	# show bison parsing trace
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp -I. -DYYDEBUG -std=c++20

miniGo:
	@echo "--- Build Compiler ---"
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp -I. -std=c++20

test: clean miniGo
	@echo "--- Build Runtime ---"
	clang -S -emit-llvm runtime.c -o runtime.o.ll
	@echo "--- Build Main ---"
	./miniGo.out ../debug/main.go -o ../debug/main.o.ll
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
