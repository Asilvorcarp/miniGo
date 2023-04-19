.PHONY : all clean

all: miniGo


miniGo:
	bison --yacc -dv miniGo.y
	flex miniGo.l
	gcc -o miniGo.out y.tab.c lex.yy.c -lm

test: miniGo
	@echo
	@echo "--- Start Calc Test ---"	
	@cat test.in | ./miniGo.out


clean:
	-rm *.out
	-rm *.yy.c
	-rm *.tab.c
	-rm *.tab.h
	-rm *.output
