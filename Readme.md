# miniGo

The compiler for a subset of Go, implemented in C++.

## grammar

// TODO

keywords:
```
"package"
"import"
"var"
"func"
"return"
"if"
"else"
"for"
"break"
"continue"
"defer"
"goto"
```

## build

```bash
cmake -S "repo目录" -B "build目录" -DLIB_DIR="libkoopa目录" -DINC_DIR="libkoopa头文件目录"
cmake --build "build目录" -j `nproc`
```

## EBNF

```c++
Exp           : LOrExp;
LVal          : IDENT
              | LVal '[' Exp ']' ;
PrimaryExp    : '(' Exp ')' | LVal | Number;
UnaryExp      : PrimaryExp 
              | IDENT '(' ')' 
              | IDENT '(' FuncRParams ')' 
              | UnaryOp UnaryExp;
UnaryOp       : '+' | '-' | NOT;
FuncRParams   : Exp | FuncRParams ',' Exp;
MulExp        : UnaryExp | MulExp MulOp UnaryExp;
MulOp         : '*' | '/' | '%';
AddExp        : MulExp | AddExp AddOp MulExp;
AddOp         : '+' | '-';
RelExp        : AddExp | RelExp RelOp AddExp;
RelOp         : LT | GT | LE | GE;
EqExp         : RelExp | EqExp EqOp RelExp;
EqOp          : EQ | NE;
LAndExp       : EqExp | LAndExp AND EqExp;
LOrExp        : LAndExp | LOrExp OR LAndExp;
ConstExp      : Exp;








CompUnit      ::= [CompUnit] (Decl | FuncDef);

Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
ConstInitVal  ::= ConstExp | "{" [ConstInitVal {"," ConstInitVal}] "}";
VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDef        ::= IDENT {"[" ConstExp "]"}
                | IDENT {"[" ConstExp "]"} "=" InitVal;
InitVal       ::= Exp | "{" [InitVal {"," InitVal}] "}";

FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
FuncType      ::= "void" | "int";
FuncFParams   ::= FuncFParam {"," FuncFParam};
FuncFParam    ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;
Stmt          ::= LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "if" "(" Exp ")" Stmt ["else" Stmt]
                | "while" "(" Exp ")" Stmt
                | "break" ";"
                | "continue" ";"
                | "return" [Exp] ";";

Exp           ::= LOrExp;
LVal          ::= IDENT {"[" Exp "]"};
PrimaryExp    ::= "(" Exp ")" | LVal | Number;
Number        ::= INT_CONST;
UnaryExp      ::= PrimaryExp | IDENT "(" [FuncRParams] ")" | UnaryOp UnaryExp;
UnaryOp       ::= "+" | "-" | "!";
FuncRParams   ::= Exp {"," Exp};
MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp       ::= EqExp | LAndExp "&&" EqExp;
LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
ConstExp      ::= Exp;

```
