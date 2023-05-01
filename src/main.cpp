#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <ast.hpp>

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler input -o output
    // compiler input

    string input, output;
    if (argc == 4) {
        assert(argv[2] == string("-o"));
        input = argv[1];
        output = argv[3];  // not used yet
    } else if (argc == 2) {
        input = argv[1];
        output = "a.out";
    } else {
        assert(0);
    }

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input.c_str(), "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;

    cout<<">> parsing... ";
    auto ret = yyparse(ast);
    cout<<"done"<<endl;

    assert(!ret);

    // 输出解析得到的 AST, 其实就是个字符串
    cout << *ast << endl;
    return 0;
}
