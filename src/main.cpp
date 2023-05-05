#include <AST.hpp>
#include <Compiler.hpp>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

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

    // // print ast as json
    // cout << ">> ast: " << endl;
    // cout << *ast << endl;

    // output ast to json file
    FILE *astFp = fopen("ast.o.json", "w");
    fprintf(astFp, "%s", ast->toJson().dump(4).c_str());
    fclose(astFp);

    // >> compile to .ll
    auto compiler = Compiler();

#ifdef YYDEBUG
    compiler.debug = true;
#endif
    auto unit = reinterpret_cast<CompUnitAST *>(ast.get());
    string ll = compiler.Compile(unit);

    // output ast with dynamic information to json file
    FILE *astDFp = fopen("ast.o.json", "w");
    fprintf(astDFp, "%s", ast->toJson().dump(4).c_str());
    fclose(astDFp);

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
        output = "a.ll";
    } else {
        assert(0);
    }

    build(input, output);

    return 0;
}
