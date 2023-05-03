#pragma once

#include <AST.hpp>
#include <Scope.hpp>

using namespace std;

// TODO remove ugo!
const static string Header = R"(
declare i32 @ugo_builtin_println(i32)
declare i32 @ugo_builtin_exit(i32)
)";

const string MainMain = R"(
define i32 @main() {
	call i32() @ugo_main_init()
	call i32() @ugo_main_main()
	ret i32 0
}
)";

class Compiler {
   public:
    CompUnitAST* file;
    Scope* scope;
    int nextId;

    Compiler() : scope(Scope::Universe()) {}
    ~Compiler() {}

    void genHeader(CompUnitAST* file, ostream& os);
    void compileFile(CompUnitAST* file, ostream& os);
    void genMain(CompUnitAST* file, ostream& os);

    string Compile(CompUnitAST* _file) {
        stringstream ss;

        file = _file;

        genHeader(file, ss);
        compileFile(file, ss);
        genMain(file, ss);

        return ss.str();
    }

    void enterScope() { scope = new Scope(scope); }
    void leaveScope() { scope = scope->Outer; }
    void restoreScope(Scope* s) { scope = s; }

    void genHeader(CompUnitAST* file, ostream& os) {
        os << "; package " << file->packageName << endl;
        os << Header << endl;
    }

    void genMain(CompUnitAST* file, ostream& os) {
        if (file->packageName != "main") {
            return;
        }
        for (auto& funcDef : file->Funcs) {
            if (funcDef->ident == "main") {
                os << MainMain;
                return;
            }
        }
    }

    void genInit(CompUnitAST* file, ostream& os) {
        os << "define i32 @ugo_" << file->packageName << "_init() {\n";

        for (auto& g : file->Globals) {
            string localName = "0";
            // doing TODO
        }
    }
};