.PHONY : all clean

all: miniGo

miniGo:
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp -I. -DYYDEBUG -std=c++20

test: clean miniGo
	@echo "--- Build runtime ---"	
	clang -S -emit-llvm runtime.c -o runtime.o.ll
	@echo "--- Build main ---"	
	./miniGo.out ../debug/main.go -o ../debug/main.o.ll
	clang runtime.o.ll ../debug/main.o.ll -o ../debug/main.out
	@echo "--- Start Main ---"	
	@../debug/main.out

clean:
	-rm lexer.cpp
	-rm parser.cpp
	-rm parser.hpp
	-rm *.yy.*
	-rm *.tab.*
	-rm *.o.ll
	-rm *.out
	-rm *.output
	-rm ../debug/*.o.ll
	-rm ../debug/*.out
