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

    void genHeader(ostream& os, CompUnitAST* file);
    void compileFile(ostream& os, CompUnitAST* file);
    void genMain(ostream& os, CompUnitAST* file);

    string Compile(CompUnitAST* _file) {
        stringstream ss;

        file = _file;

        genHeader(ss, file);
        compileFile(ss, file);
        genMain(ss, file);

        return ss.str();
    }

    void enterScope() { scope = new Scope(scope); }
    void leaveScope() { scope = scope->Outer; }
    void restoreScope(Scope* s) { scope = s; }

    void genHeader(ostream& os, CompUnitAST* file) {
        os << "; package " << file->packageName << endl;
        os << Header << endl;
    }

    void genMain(ostream& os, CompUnitAST* file) {
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

    void compileFile(ostream& os, CompUnitAST* file);
    void compileFunc(ostream& os, CompUnitAST* file, FuncDefAST* fn);
    void compileStmt(ostream& os, StmtAST* stmt);
    void compileStmt_assign(ostream& os, AssignStmtAST* stmt);
    string compileExpr(ostream& os, ExpAST* expr);
    string genId() {
        stringstream ss;
        ss << "%t" << nextId++;
        return ss.str();
    }
    string genLabelId(string name) {
        stringstream ss;
        ss << name << "." << nextId++;
        return ss.str();
    }

    constexpr uint32_t hash(const string& s) noexcept {
        uint32_t hash = 5381;
        for (int i = 0; i <= s.size(); ++i)
            hash = ((hash << 5) + hash) + (unsigned char)s[i];
        return hash;
    }

    void genInit(ostream& os, CompUnitAST* file) {
        os << "define i32 @ugo_" << file->packageName << "_init() {\n";

        for (auto& g : file->Globals) {
            string localName = "0";
            if (g->initVals->size() > 0) {
                // TODO doing
                // localName = compileExpr(os, *(*g->initVals)[0]);
            }
        }
    }

    string compileExpr(ostream& os, ExpAST* expr) {
        string varName;
        switch (expr->type()) {
            case TType::LValT:
                auto exp = reinterpret_cast<LValAST*>(expr);
                varName = exp->ident;
                break;
            case TType::NumberT:
                auto exp = reinterpret_cast<NumberAST*>(expr);
                break;
            case TType::BinExpT:
                auto exp = reinterpret_cast<BinExpAST*>(expr);
                break;
            case TType::UnaryExpT:
                auto exp = reinterpret_cast<UnaryExpAST*>(expr);
                break;
            case TType::ParenExpT:
                auto exp = reinterpret_cast<ParenExpAST*>(expr);
                break;
            case TType::CallExpT:
                auto exp = reinterpret_cast<CallExpAST*>(expr);
                break;
            default:
                break;
        }
    }
};