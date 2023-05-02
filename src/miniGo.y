%code requires {
  #include <vector>
  #include <memory>
  #include <string>
  #include <AST.hpp>
}

%{

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <AST.hpp>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(pAST &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { pAST &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  vpAST *ast_list;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN PACKAGE IMPORT IF ELSE FOR DEFINE
    WHILE BREAK CONTINUE DEFER GOTO VAR FUNC CONST
    LT GT LE GE EQ NE AND OR NOT INC DEC
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> PackClause FuncDef FuncType Block Stmt ReturnStmt Exp ExpStmt IncDecStmt AssignStmt ShortVarDecl LVal IDs InitVals FuncFParam BType TopLevelDecl PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <int_val> Number
%type <ast_list> TopLevelDeclList FuncFParamList StmtList ArgList

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
// A source file is a compilation unit
// !! must be the first symbol
CompUnit : PackClause TopLevelDeclList {
    // ignore package
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->topDefs = pvpAST($2);
    ast = std::move(comp_unit);
};

TopLevelDecl : Decl | FuncDef {
    $$ = $1;
};
TopLevelDeclList : /* empty */ {
    $$ = new vpAST();
} | TopLevelDecl {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | TopLevelDeclList TopLevelDecl {
    auto l = $1;
    l->push_back(pAST($2));
    $$ = l;
};
PackClause : PACKAGE IDENT {
    auto ast = new PackClauseAST();
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
};

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
// TODO: void func type, param list

FuncFParam : IDENT BType {

};

FuncFParamList : /* empty */ {
    $$ = new vpAST();
} | FuncFParam {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | FuncFParamList ',' FuncFParam {
    auto l = $1;
    l->push_back(pAST($3));
    $$ = l;
};

FuncDef : FUNC IDENT '(' FuncFParamList ')' FuncType Block {
    auto ast = new FuncDefAST();
    ast->ident = *unique_ptr<string>($2);
    ast->func_type = pAST($6);
    ast->block = pAST($7);
    $$ = ast;
};

FuncType : /* empty */ {
    auto ast = new FuncTypeAST();
    // default void type
    $$ = ast;
} | BType {
    auto ast = new FuncTypeAST();
    ast->type = "int"; // TODO
    $$ = ast;
};

Block : '{' StmtList '}' {
    auto ast = new BlockAST();
    ast->stmts = pvpAST($2);
    $$ = ast;
};

StmtList : /* empty */ {
    auto l = new vpAST();
    $$ = l;
} | Stmt {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | StmtList Stmt {
    auto l = $1;
    l->push_back(pAST($2));
    $$ = l;
};

ExpStmt : Exp;
IncDecStmt : LVal INC {
    // TODO
} | LVal DEC {
    // TODO
};
AssignStmt : LVal '=' Exp {
    // TODO
};
// i, j := 0, 10
ShortVarDecl : IDs DEFINE InitVals;
SimpleStmt : /* empty stmt */ {

} | ExpStmt {

} | IncDecStmt {

} | AssignStmt {

} | ShortVarDecl {

};

/*
Statement =
	Declaration | LabeledStmt | SimpleStmt |
	GoStmt | ReturnStmt | BreakStmt | ContinueStmt | GotoStmt |
	FallthroughStmt | Block | IfStmt | SwitchStmt | SelectStmt | ForStmt |
	DeferStmt .
*/
//IfStmt = "if" [ SimpleStmt ";" ] Expression Block [ "else" ( IfStmt | Block ) ] .
IfStmt : IF Exp Block {
    // TODO
} | IF Exp Block ELSE IfStmt {
    // TODO
} | IF Exp Block ELSE Block {
    // TODO
} | IF SimpleStmt ';' Exp Block {
    // TODO
} | IF SimpleStmt ';' Exp Block ELSE IfStmt {
    // TODO
} | IF SimpleStmt ';' Exp Block ELSE Block {
    // TODO
};
ReturnStmt : RETURN Exp {
    auto ast = new ReturnStmtAST();
    ast->exp = pAST($2);
    $$ = ast;
};
ForStmt : FOR Block { // always
    // TODO
} | FOR Exp Block { // while
    // TODO
} | FOR SimpleStmt ';' Exp ';' SimpleStmt Block { // for
    // TODO
};
Stmt : Decl {

} | IfStmt {

} | ReturnStmt {

} | SimpleStmt {
    //TODO
};

// var i, j int = 1, 2
// var c, python, java = true, false, "no!"
// var (
//	ToBe   bool       = false
//	MaxInt uint64     = 1<<64 - 1
//	z      complex128 = cmplx.Sqrt(-5 + 12i)
// )
// array: var a [2]type
// TODO now only support one var decl
IDs : IDENT | IDs ',' IDENT;
InitVal : Exp | '{' InitVals '}';
InitVals : InitVal | InitVals ',' InitVal;
ConstInitVal  : ConstExp | '{' ConstInitVals '}';
ConstInitVals : ConstInitVal | ConstInitVals ',' ConstInitVal;
VarDecl : VAR IDs BType '=' InitVals
        | VAR IDs BType;
VarSpec : IDs BType '=' InitVals
        | IDs '=' InitVals
        | IDs ;
ConstDecl : CONST ConstSpec;
ConstSpec : IDs BType '=' ConstInitVals
          | IDs '=' ConstInitVals
          | IDs;
// TODO TypeDecl
Decl : VarDecl | ConstDecl;
ConstIndex : '[' ConstExp ']';
ConstIndexs : ConstIndex | ConstIndexs ConstIndex;
BType : INT | INT ConstIndexs;

Number : INT_CONST {
    $$ = $1;
};
Exp : LOrExp {
    $$ = $1;
};
LVal : IDENT {
} | LVal '[' Exp ']' {
};
PrimaryExp : '(' Exp ')' {
    $$ = new PrimaryExpAST($2, PrimaryExpAST::Type::PAREN);
} | LVal {
    $$ = new PrimaryExpAST($1, PrimaryExpAST::Type::LVAL);
} | Number{
    $$ = new PrimaryExpAST($1);
};
UnaryExp : PrimaryExp  {
    $$ = $1;
} | IDENT '(' ArgList ')' {
} | UnaryOp UnaryExp;
UnaryOp: '+' | '-' | NOT;
ArgList : /* empty */ {
    auto l = new vpAST();
    $$ = l;
} | Exp {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | ArgList ',' Exp {
    auto l = $1;
    l->push_back(pAST($3));
    $$ = l;
};
MulExp: UnaryExp {
    $$ = $1;
} | MulExp MulOp UnaryExp;
MulOp: '*' | '/' | '%';
AddExp: MulExp {
    $$ = $1;
} | AddExp AddOp MulExp;
AddOp: '+' | '-';
RelExp: AddExp {
    $$ = $1;
} | RelExp RelOp AddExp;
RelOp: LT | GT | LE | GE;
EqExp: RelExp {
    $$ = $1;
} | EqExp EqOp RelExp;
EqOp: EQ | NE;
LAndExp: EqExp {
    $$ = $1;
} | LAndExp AND EqExp;
LOrExp: LAndExp {
    $$ = $1;
} | LOrExp OR LAndExp;
ConstExp: Exp {
    // TODO judge exp is const, by get value?
    $$ = $1;
};


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(pAST &ast, const char *s) {
  cerr << "error: " << s << endl;
}

// run with trace
// make && ./miniGo.out ../debug/return.go && make clean