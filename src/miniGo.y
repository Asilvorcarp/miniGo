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
  char char_val;
  BaseAST *ast_val;
  vpAST *ast_list;
  vector<string> *str_list;
  vector<int> *int_list;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN PACKAGE IMPORT IF ELSE FOR DEFINE
    BREAK CONTINUE DEFER GOTO VAR FUNC CONST
    INC DEC MAKE NIL
%token <str_val> IDENT LE GE EQ NE AND OR BIN_ASSIGN
%token <int_val> INT_CONST
%token <char_val> CHAR_CONST '+' '-' '*' '/' '%' '!' '&' '|' '^' '<' '>' '=' 

// 非终结符的类型定义
%type <ast_val> FuncDef ReturnType Block Stmt ReturnStmt Exp ExpStmt IncDecStmt AssignStmt ShortVarDecl LVal Param BType TopLevelDecl PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp VarDecl ConstDecl VarSpec ConstSpec ForStmt SimpleStmt Decl IfStmt InitVal BranchStmt
%type <int_val> Number ConstIndex ConstExp ConstInitVal
%type <char_val> AddOp MulOp UnaryOp
%type <str_val> RelOp EqOp PackClause
%type <ast_list> TopLevelDeclList ParamList StmtList ArgList InitVals InitValList LVals
%type <str_list> IDs
%type <int_list> ConstIndexList ConstInitVals

// - about their emptyness -
// if something cannot be empty, then
//   somethings cannot be empty (at least one)
//   somethingList can be empty (zero or more)

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
// A source file is a compilation unit
// !! must be the first symbol
CompUnit : PackClause TopLevelDeclList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->packageName = *unique_ptr<string>($1);
    comp_unit->setDefs($2);
    ast = std::move(comp_unit);
};

TopLevelDecl : Decl | FuncDef;
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
    $$ = $2;
};

// FuncDef ::= ReturnType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, ReturnType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
// TODO: void func type, param list

Param : IDENT BType {
    auto ast = new ParamAST($1, $2);
    $$ = ast;
};

ParamList : /* empty */ {
    $$ = new vpAST();
} | Param {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | ParamList ',' Param {
    auto l = $1;
    l->push_back(pAST($3));
    $$ = l;
};

FuncDef : FUNC IDENT '(' ParamList ')' ReturnType Block {
    auto ast = new FuncDefAST($2, $4, $6, $7);
    $$ = ast;
};

ReturnType : /* empty */ {
    auto ast = new BTypeAST();
    ast->elementType = "void";
    $$ = ast;
} | BType;

Block : '{' StmtList '}' {
    auto ast = new BlockAST($2);
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

ExpStmt : Exp {
    auto ast = new ExpStmtAST($1);
    $$ = ast;
};
IncDecStmt : LVal INC {
    auto ast = new IncDecStmtAST($1, true);
    $$ = ast;
} | LVal DEC {
    auto ast = new IncDecStmtAST($1, false);
    $$ = ast;
};
// TODO maybe combine it with ShortVarDecl
AssignStmt : LVals '=' InitVals {
    auto ast = new ShortVarDeclAST(false, $1, $3);
    $$ = ast;
} | LVal BIN_ASSIGN InitVal {
    char op = (*$2)[0];
    $$ = new ShortVarDeclAST($1, op, $3);
};
// i, j := 0, 10
ShortVarDecl : LVals DEFINE InitVals {
    auto ast = new ShortVarDeclAST(true, $1, $3);
    $$ = ast;
};
SimpleStmt : /* empty stmt */ {$$ = new EmptyStmtAST();} 
           | ExpStmt
           | IncDecStmt 
           | AssignStmt 
           | ShortVarDecl 
           ;

/*
Statement =
	Declaration | LabeledStmt | SimpleStmt |
	GoStmt | ReturnStmt | BreakStmt | ContinueStmt | GotoStmt |
	FallthroughStmt | Block | IfStmt | SwitchStmt | SelectStmt | ForStmt |
	DeferStmt .
*/
//IfStmt = "if" [ SimpleStmt ";" ] Expression Block [ "else" ( IfStmt | Block ) ] .
IfStmt : IF Exp Block {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::If, nullptr, $2, $3, nullptr);
    $$ = ast;
} | IF Exp Block ELSE Block {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::IfElse, nullptr, $2, $3, $5);
    $$ = ast;
} | IF Exp Block ELSE IfStmt {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::IfElseIf, nullptr, $2, $3, $5);
    $$ = ast;
} | IF SimpleStmt ';' Exp Block {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::If, $2, $4, $5, nullptr);
    $$ = ast;
} | IF SimpleStmt ';' Exp Block ELSE Block {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::IfElse, $2, $4, $5, $7);
    $$ = ast;
} | IF SimpleStmt ';' Exp Block ELSE IfStmt {
    auto ast = new IfStmtAST(
        IfStmtAST::Type::IfElseIf, $2, $4, $5, $7);
    $$ = ast;
};
ReturnStmt : RETURN Exp {
    auto ast = new ReturnStmtAST($2);
    $$ = ast;
} | RETURN {
    auto ast = new ReturnStmtAST(nullptr);
    $$ = ast;
};
// BREAK, CONTINUE, GOTO
BranchStmt : BREAK {
    auto ast = new BranchStmtAST(
        BranchStmtAST::Type::Break);
    $$ = ast;
} | CONTINUE {
    auto ast = new BranchStmtAST(
        BranchStmtAST::Type::Continue);
    $$ = ast;
} | GOTO IDENT {
    auto ast = new BranchStmtAST(
        BranchStmtAST::Type::Goto, $2);
    $$ = ast;
};
ForStmt : FOR Block { // always
    auto ast = new ForStmtAST(
        nullptr, nullptr, nullptr, $2);
    $$ = ast;
} | FOR Exp Block { // while
    auto ast = new ForStmtAST(
        nullptr, $2, nullptr, $3);
    $$ = ast;
} | FOR SimpleStmt ';' Exp ';' SimpleStmt Block { // for
    auto ast = new ForStmtAST(
        $2, $4, $6, $7);
    $$ = ast;
};
Stmt : Decl | IfStmt | ReturnStmt | SimpleStmt | ForStmt | Block | BranchStmt ;

// var i, j int = 1, 2
// var c, python, java = true, false, "no!"
// var (
//	ToBe   bool       = false
//	MaxInt uint64     = 1<<64 - 1
//	z      complex128 = cmplx.Sqrt(-5 + 12i)
// )
// array: var a [2]type
// TODO now only support one var decl
IDs : IDENT {
    auto l = new vector<string>();
    l->push_back(*unique_ptr<string>($1));
    $$ = l;
} | IDs ',' IDENT {
    auto l = $1;
    l->push_back(*unique_ptr<string>($3));
    $$ = l;
};
// initVal can be exp, array exp, make exp
InitVal : BType '{' InitValList '}' {
    auto ast = new ArrayExpAST($1, $3);
    $$ = ast;
} | MAKE '(' BType ',' Exp ')' {
    $$ = new MakeExpAST($3, $5);
} | Exp;
InitVals : InitVal { // just Exps
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | InitVals ',' InitVal {
    auto l = $1;
    l->push_back(pAST($3));
    $$ = l;
};
InitValList : /* empty */ {
    auto l = new vpAST();
    $$ = l;
} | InitVals;
ConstInitVal  : ConstExp ;
ConstInitVals : ConstInitVal {
    auto l = new vector<int>();
    l->push_back($1);
    $$ = l;
} | ConstInitVals ',' ConstInitVal {
    auto l = $1;
    l->push_back($3);
    $$ = l;
};
// TODO support multiple var spec
VarDecl : VAR VarSpec {
    $$ = $2;
};
VarSpec : IDs BType '=' InitVals {
    auto ast = new VarSpecAST($1, $2, $4);
    $$ = ast;
}| IDs '=' InitVals {
    auto ast = new VarSpecAST($1, nullptr, $3);
    $$ = ast;
}| IDs BType {
    auto ast = new VarSpecAST($1, $2, nullptr);
    $$ = ast;
};
ConstDecl : CONST ConstSpec {
    $$ = $2;
};
ConstSpec : IDs BType '=' ConstInitVals {
}| IDs '=' ConstInitVals {
};
// TODO TypeDecl
Decl : VarDecl | ConstDecl;
ConstIndex : '[' ConstExp ']'{
    $$ = $2;
} | '[' ']' {
    $$ = -1;
};
ConstIndexList : {
    $$ = new vector<int>();
} | ConstIndexList ConstIndex {
    auto l = $1;
    l->push_back($2);
    $$ = l;
};
BType : ConstIndexList INT {
    auto ast = new BTypeAST();
    // ast->type = "int"; // default
    ast->dims = unique_ptr<vector<int>>($1);
    $$ = ast;
};

Number : INT_CONST | CHAR_CONST {
    $$ = (int)$1;
};
Exp : LOrExp;
LVal : IDENT {
    auto ast = new LValAST($1, nullptr);
    $$ = ast;
} | LVal '[' Exp ']' {
    auto ast = $1;
    ast->add($3);
    $$ = ast;
};
LVals : LVal {
    auto l = new vpAST();
    l->push_back(pAST($1));
    $$ = l;
} | LVals ',' LVal {
    auto l = $1;
    l->push_back(pAST($3));
    $$ = l;
};
PrimaryExp : '(' Exp ')' {
    $$ = new ParenExpAST($2);
} | LVal | Number {
    $$ = new NumberAST($1);
} | NIL {
    $$ = new NilAST();
};
UnaryExp : PrimaryExp | IDENT '(' ArgList ')' {
    $$ = new CallExpAST($1, $3);
} | UnaryOp UnaryExp {
    $$ = new UnaryExpAST($1, $2);
};
UnaryOp: '+' | '-' | '!';
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
MulExp: UnaryExp | MulExp MulOp UnaryExp {
    $$ = new BinExpAST($2, $1, $3);
};
MulOp: '*' | '/' | '%';
AddExp: MulExp | AddExp AddOp MulExp {
    $$ = new BinExpAST($2, $1, $3);
};
AddOp: '+' | '-';
RelExp: AddExp | RelExp RelOp AddExp {
    $$ = new BinExpAST($2, $1, $3);
};
RelOp: '<' {$$=new string(yytext);} | '>' {$$=new string(yytext);} 
     | LE | GE;
EqExp: RelExp | EqExp EqOp RelExp {
    $$ = new BinExpAST($2, $1, $3);
};
EqOp: EQ | NE;
LAndExp: EqExp | LAndExp AND EqExp {
    $$ = new BinExpAST($2, $1, $3);
};
LOrExp: LAndExp | LOrExp OR LAndExp {
    $$ = new BinExpAST($2, $1, $3);
};
ConstExp: Exp {
    // error if not const
    auto exp = reinterpret_cast<ExpAST*>($1);
    $$ = exp->eval();
};


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(pAST &ast, const char *s) {
  cerr << "error: " << s << endl;
}
