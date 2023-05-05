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

## Libaraies

Json Library:
https://github.com/nlohmann/json

## Doc

### dynamic array

(LLVM refers to [LLVM IR](https://llvm.org/docs/LangRef.html) below)

Dynamic array is implemented with `alloca` in LLVM.
The template of alloca: `pT1 = alloca T1, i32 numOfElements, align 4`.
Any array of any dimension in the generated LLVM IR is just a `ptr`.
(if T1 is i32, pT1 is ptr; if T1 is ptr, pT1 is still ptr)
Only the compiler knows the dimension of the array, 
through methods like `Compiler::inferType(pAST exp)` and `BaseAST::info()`.

This is written as `make(Type t, int size)` function in Golang.

For example, the following code:

```go

arr := make([][]int, 4)
arr[0] = make([]int, 5)
arr[1] = make([]int, 5)
arr[1][1] = 110
putint(arr[1][1])

```

is translated to:

```llvm

; array of [4 x [5 x i32]] as ptr
%spaa = alloca ptr, align 4 ; done by assign
; template: pT1 = alloca T1, i32 numOfElements, align 4
%paa = alloca ptr, i32 4, align 4 ; done by make
store ptr %paa, ptr %spaa, align 4

%pa0 = alloca i32, i32 5, align 4
    %paa_0 = load ptr, ptr %spaa, align 4
    %spa0 = getelementptr inbounds ptr, ptr %paa_0, i32 0 ; index arr[0]
store ptr %pa0, ptr %spa0, align 4

%pa1 = alloca i32, i32 5, align 4
%paa_1 = load ptr, ptr %spaa, align 4
%spa1 = getelementptr inbounds ptr, ptr %paa_1, i32 1 ; index arr[1]
store ptr %pa1, ptr %spa1, align 4
; init done

; set 110 to arr[1][1]
%sp11 = getelementptr inbounds ptr, ptr %pa1, i32 1 ; index arr[1][1]
store i32 110, ptr %sp11, align 4 ; set to 110

; now get the 110 from %spaa
%paa_ = load ptr, ptr %spaa, align 4
%spa1_ = getelementptr inbounds ptr, ptr %paa_, i32 1 ; index arr[1]
%pa1_ = load ptr, ptr %spa1_, align 4
%sp11_ = getelementptr inbounds ptr, ptr %pa1_, i32 1 ; index arr[1][1]
%sp11_val = load i32, ptr %sp11_, align 4
call i32 @runtime_putint(i32 %sp11_val)

```

For more details, see `debug/array.ll`.

### runtime functions

The functions like `println` and `getchar` are implemented in `runtime.c`.
They act like the standard library or the runtime of Golang.

### type inference

Type inference is needed because of statements like `i := arr[1]`.
Some simple type inference is done by `Compiler::inferType(pAST exp)` and `BaseAST::info()`. 

The former is used to infer the type of a expression,
usually for the right hand side of a assignment.

The latter is often used to get the type of some part of a statement,
so that the compiler can generate the correct LLVM IR.
This includes the type of a variable in a declaration,
the type of a function parameter in a function definition,
the type of the expression in a return statement, etc.

### type checking

Some simple type checking is done inside of some methods of the compiler, including:

When indexing an array, the index expression should be of type `int`.
If not, the compiler would throw an exception, like
`compileExpr: index expression is not int`.

When `compileExpr` compiles a expression of type `CallExpAST` (function call),
it would check if the number of arguments matches the number of parameters,
and if the type of each argument matches the type of the corresponding parameter.
If not, the compiler would throw an exception, like
`compileExpr: type mismatch in function call - FuncName`
` - arg "a" has type "int", but expected "[]int"`.

Todo:
when `compileStmt` compiles a statement of type `ReturnStmtAST`,
it would check if the type of the expression matches the return type of the function.
// Techniques like finding the func containing the return statement are needed.

### const expression

Const expression is evaluated at compile time with `int ExpAST::eval()`.
The compiler would throw an exception if the expression is not const.

Todo: const variable is not supported yet.

### some features

Todo: `defer` in Golang

Todo: 
multi-assignment in Golang
like swap by `a, b = b, a`
