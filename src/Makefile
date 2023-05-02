.PHONY : all clean

all: miniGo

miniGo:
	flex -o miniGo.yy.cpp miniGo.l
	bison -t miniGo.y -o miniGo.tab.hpp
	clang++ -o miniGo.out miniGo.yy.cpp main.cpp -I. -DYYDEBUG -std=c++17

test: miniGo
	@echo
	@echo "--- Start Calc Test ---"	
	@cat test.in | ./miniGo.out

clean:
	-rm lexer.cpp
	-rm parser.cpp
	-rm parser.hpp
	-rm *.out
	-rm *.yy.*
	-rm *.tab.*
	-rm *.output


