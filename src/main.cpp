#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <AST.hpp>
#include <Compiler.hpp>

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
extern int yydebug;

// .go to .ll
void build(string inFile, string outLL) {
    // >> scan and parse
#ifdef YYDEBUG
    yydebug = 1;
#endif
    yyin = fopen(inFile.c_str(), "r");
    assert(yyin);

    unique_ptr<BaseAST> ast;

    cout << ">> parsing... " << endl;
    auto ret = yyparse(ast);
    cout << ">> done" << endl;

    assert(!ret);

    // print ast as json
    cout << *ast << endl;

    // >> compile to .ll
    auto compiler = Compiler();
    auto unit = reinterpret_cast<CompUnitAST *>(ast.get());
    string ll = compiler.Compile(unit);

    cout << ">> ll: " << endl;
    cout << ll << endl;

    // print ll to file output
    FILE *fp = fopen(outLL.c_str(), "w");
    fprintf(fp, "%s", ll.c_str());
    fclose(fp);
    
    // after this,
    // clang outLL runtime.ll -o a.out
}

int main(int argc, const char *argv[]) {
    // compiler input -o output
    // compiler input

    string input, output;
    if (argc == 4) {
        assert(argv[2] == string("-o"));
        input = argv[1];
        output = argv[3];
    } else if (argc == 2) {
        input = argv[1];
        output = "a.out";
    } else {
        assert(0);
    }

    build(input, output);

    return 0;
}
