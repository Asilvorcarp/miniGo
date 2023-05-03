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
    // stmt: *ShortVarDeclAST
    void compileStmt_assign(ostream& os, pAST& _stmt);
    // expr: *ExpAST
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

    void compileFile(ostream& os, CompUnitAST* file) {}
    void compileFunc(ostream& os, CompUnitAST* file, FuncDefAST* fn) {}
    void compileStmt(ostream& os, pAST& _stmt) {}
    void compileStmt_assign(ostream& os, pAST& _stmt) {
        // stmt: *ShortVarDeclAST
        auto stmt = reinterpret_cast<ShortVarDeclAST*>(_stmt.get());
        auto& targets = *stmt->targets;
        auto& initVals = *stmt->initVals;
        vector<string> valueNameList(initVals.size());
        for (int i = 0; i < (*stmt->targets).size(); ++i) {
            valueNameList[i] = compileExpr(os, initVals[i]);
        }
        if (stmt->isDefine == true) {
            //TODO doing
        }
    }

    string compileExpr(ostream& os, pAST& _expr) {
        // expr: *ExpAST
        auto expr = reinterpret_cast<ExpAST*>(_expr.get());
        string localName;  // ret
        string varName;
        string funcName;
        switch (expr->type()) {
            case TType::LValT:
                auto exp0 = reinterpret_cast<LValAST*>(expr);
                auto obj = scope->Lookup(exp0->ident).second;
                if (obj != nullptr) {
                    varName = obj->MangledName;
                } else {
                    cerr << "var " << exp0->ident << " undefined" << endl;
                    assert(false);
                }
                localName = genId();
                os << "\t" << localName << " = load i32, i32* " << varName
                   << ", align 4\n";
                return localName;
                break;
            case TType::NumberT:
                auto exp1 = reinterpret_cast<NumberAST*>(expr);
                localName = genId();
                os << "\t" << localName << " = "
                   << "add"
                   << " i32 "
                   << "0"
                   << ", " << exp1->num << endl;
                return localName;
                break;
            case TType::BinExpT:
                auto exp2 = reinterpret_cast<BinExpAST*>(expr);
                localName = genId();
                switch (exp2->op) {
                    case BinExpAST::Op::ADD:
                        os << "\t" << localName << " = "
                           << "add"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::SUB:
                        os << "\t" << localName << " = "
                           << "sub"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::MUL:
                        os << "\t" << localName << " = "
                           << "mul"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::DIV:
                        os << "\t" << localName << " = "
                           << "div"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::MOD:
                        os << "\t" << localName << " = "
                           << "srem"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                        // https://llvm.org/docs/LangRef.html#icmp-instruction
                    case BinExpAST::Op::EQ:
                        os << "\t" << localName << " = "
                           << "icmp eq"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::NE:
                        os << "\t" << localName << " = "
                           << "icmp ne"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::LT:
                        os << "\t" << localName << " = "
                           << "icmp slt"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::LE:
                        os << "\t" << localName << " = "
                           << "icmp sle"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::GT:
                        os << "\t" << localName << " = "
                           << "icmp sgt"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    case BinExpAST::Op::GE:
                        os << "\t" << localName << " = "
                           << "icmp sge"
                           << " i32 " << compileExpr(os, exp2->left) << ", "
                           << compileExpr(os, exp2->right) << endl;
                        return localName;
                        break;
                    default:
                        cerr << "compileExpr: unknown type of BinExpAST"
                             << endl;
                        assert(false);
                        break;
                }
                break;
            case TType::UnaryExpT:
                auto exp3 = reinterpret_cast<UnaryExpAST*>(expr);
                if (exp3->op == '-') {
                    localName = genId();
                    os << "\t" << localName << " = "
                       << "sub"
                       << " i32 "
                       << "0"
                       << ", " << compileExpr(os, exp3->p) << endl;
                    return localName;
                }
                return compileExpr(os, exp3->p);
                break;
            case TType::ParenExpT:
                auto exp4 = reinterpret_cast<ParenExpAST*>(expr);
                return compileExpr(os, exp4->p);
                break;
            case TType::CallExpT:
                auto exp5 = reinterpret_cast<CallExpAST*>(expr);
                auto obj = scope->Lookup(exp5->funcName).second;
                if (obj != nullptr) {
                    funcName = obj->MangledName;
                } else {
                    cerr << "compileExpr: function " << exp5->funcName
                         << " undefined" << endl;
                    assert(false);
                }
                localName = genId();
                os << "\t" << localName << " = call i32(i32) " << funcName
                   << "(i32 "  // TODO test this
                   << compileExpr(os, (*exp5->argList)[0]) << ")" << endl;
                return localName;
                break;
            default:
                cerr << "compileExpr: unknown type of ExpAST" << endl;
                assert(false);
                break;
        }
        return localName;
    }
};