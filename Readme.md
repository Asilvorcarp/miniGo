# miniGo

The compiler for a subset of Golang, implemented in C++ and Python.

## Usage

```bash
# get llvm code
miniGo main.go -o main.ll
# get amd64 assembly code
src/Backend.py -f main.ll -o main.s
```

`main.go` is the Golang source code and `main.ll` is the LLVM IR output you want.

This will also generate the AST json file `ast.o.json` for debugging.
If the output filename is not specified, the default one would be `a.ll`.

## Build and Test

**To build the compiler `build/miniGo`:**
```bash
make
# Or: make miniGo 
```

To generate ll for `debug/main.go`:
```bash
make ll
```
This will also generate the AST json file ast.o.json for debugging.

To generate executable for `debug/main.go`:
```bash
make main
```

To generate and test the executable with input `debug/test.temp.in`:
```bash
make in
```

**To test on every test case in `debug/tests/X.go` with input files `debug/tests/X/*.in`:**
```bash
make tests
```

To clean up:
```bash
make clean
```

## Grammar

// TODO

### Keywords
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

### EBNF

// TODO

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
Used to output AST in json format for debugging.

Flex and Bison:
https://www.gnu.org/software/bison/
Used to generate lexer and parser.

## Doc

### Array

(LLVM refers to [LLVM IR](https://llvm.org/docs/LangRef.html) below)

Local array is implemented with `alloca` in LLVM, which is freed when the function returns. // TODO
The template of alloca: `pT1 = alloca T1, i32 numOfElements, align 4`.
Dynamic array is implemented with runtime function `malloc`.
The compiler knows the dimension of the array
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

### Runtime functions

The functions including `getchar` `putchar` and `malloc` are specified in the scope of the language, see `src/Scope.hpp`.
They act like the standard library or the runtime of Golang, and would be linked with the generated LLVM IR.

### Type inference

Type inference is needed because of statements like `i := arr[1]`.
Some simple type inference is done by `Compiler::inferType(pAST exp)` and `BaseAST::info()`. 

The former is used to infer the type of a expression,
usually for the right hand side of a assignment.

The latter is often used to get the type of some part of a statement,
so that the compiler can generate the correct LLVM IR.
This includes the type of a variable in a declaration,
the type of a function parameter in a function definition,
the type of the expression in a return statement, etc.

### Type checking

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

Relavant functions in class `Compiler`:

- `bool matchType(string t1, string t2)`: check if two types match.
- `bool isPtr(string t)`: check if a type is a pointer.
- `string reduceDim(string t)`: reduce the dimension of a type by 1.
- `string reduceDim(string t, int n)`: reduce the dimension of a type by n.
- `string increaseDim(string t)`: increase the dimension of a type by 1.
- `string inferType(pAST exp)`: infer the type of a expression.

### Const expression

Const expression is evaluated at compile time with `int ExpAST::eval()`.
The compiler would throw an exception if the expression is not const.

Todo: const variable is not supported yet.

### Equivalent AST

Some candy grammars are implemented by converting to an equivalent AST, including:

```
a += b 
-> a = a + b

a++
-> a = a + 1

a := b // TODO
-> var a = b

a := []int{1, 2, 3} // TODO
-> var a = make([]int, 3)
   a[0] = 1
   a[1] = 2
   a[2] = 3
```

### Some features

#### Multi-assignment in Golang

Multi-assignment is supported, like `a, b = c, d`.

Moreover, because temp variables are generated for each right hand side expression,
we can do cool stuff like **swapping by `a, b = b, a`**.

#### Todo: `defer` in Golang

#### Other small features

- char literal, like `'a'`
- assign with binary operators, like `a += 1`

### Test Cases

Test files of typical use are in `tests/`:

- sort.go
- matrix.go
- course_selection.go

As mentioned above, you can test all of them with `make tests`.

More detailed test cases are in `debug/`, which are used for debugging the compiler.

To compile and run with go official compiler for comparison:

```bash
go run debug/main.go debug/Runtime.go
```

Explanation on the test cases:

#### `var.go`

// TODO

#### `make.go`

// TODO

#### `for.go`

// TODO

#### `ifElse.go`

// TODO

#### `array.ll`

The LLVM code about array and pointer. It is written by hand, which helps to understand the generated LLVM IR.

To generate a similar one:

```bash
clang -S -emit-llvm -o array.ll array.c
```

To run it:

```bash
clang -o array array.ll && ./array
```

#### `recAdd.ll`

The LLVM code of recursive addition, which make the feature of recursive function call in LLVM clear.

// TODO

### Implementation Details

Golang Code -> Tokens -> AST -> LLVM IR -> Machine Code

// TODO

Scanner, Parser, Compiler, Linker(clang).

Static semantic analysis in Parser, like checking if a variable is declared before use.

Dynamic semantic analysis in Compiler, like type inference, type checking, etc.

