%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int yylex(void);
void yyerror(char *);
%}

%union{
  int inum;
//   double dnum;
}

%token ADD SUB MUL DIV POW LPAREN RPAREN CR
%token <inum> NUM
%type  <inum> expression

// order (the latter is higher)
%left ADD SUB
%left MUL DIV
%left NEG
%right POW

%%

       line_list: line
                | line_list line
                ;
				
	       line : expression CR  {printf(">>%d\n", $1);}
                | CR             {printf("\n");}
                ;
     
      expression: NUM { $$=$1; }
                | expression ADD expression { $$=$1+$3; }
                | expression SUB expression { $$=$1-$3; }
                | expression MUL expression { $$=$1*$3; }
                | expression DIV expression { $$=$1/$3; }
                | SUB expression %prec NEG  { $$=-$2; }
                | expression POW expression { $$=pow($1, $3); }
                | LPAREN expression RPAREN  { $$=$2; }
                ;

%%

void yyerror(char *str){
    fprintf(stderr,"error:%s\n",str);
}

int yywrap(){
    return 1;
}

int main(){
    yyparse();
}