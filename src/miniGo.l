/* // 这里写一些选项, 可以控制 Flex/Bison 的某些行为 */
%option noyywrap
%option nounput
%option noinput

%{

// 这里写一些全局的代码
// 因为最后要生成 C/C++ 文件, 实现主要逻辑的部分都是用 C/C++ 写的
// 难免会用到头文件, 所以通常头文件和一些全局声明/定义写在这里

#include <iostream>
#include <cstdlib>
#include <string>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "miniGo.tab.hpp"

using namespace std;

%}

/* // 这里写一些 Flex/Bison 相关的定义
// 对于 Flex, 这里可以定义某个符号对应的正则表达式
// 对于 Bison, 这里可以定义终结符/非终结符的类型 */

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }

"package"       { return PACKAGE; }
"import"        { return IMPORT; }
"var"           { return VAR; }
"func"          { return FUNC; }
"return"        { return RETURN; }
"if"            { return IF; }
"else"          { return ELSE; }
"for"           { return FOR; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }
"defer"         { return DEFER; }
"goto"          { return GOTO; }
"const"         { return CONST; }
":="            { return DEFINE; }
"++"            { return INC; }
"--"            { return DEC; }

"int"           { return INT; }

"=="            { return EQ; }
"!="            { return NE; }
"<"             { return LT; }
"<="            { return LE; }
">"             { return GT; }
">="            { return GE; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }

%%

// 这里写一些用户自定义的代码
// 比如你希望在生成的 C/C++ 文件里定义一个函数, 做一些辅助工作
// 你同时希望在之前的规则描述里调用你定义的函数
// 那么, 你可以把 C/C++ 的函数定义写在这里, 声明写在文件开头
