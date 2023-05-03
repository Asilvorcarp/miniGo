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

    Compiler() : scope(Scope::Universe()), nextId(0) {}
    ~Compiler() {}

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

    void genInit(ostream& os, CompUnitAST* file) {
        os << "define i32 @ugo_" << file->packageName << "_init() {\n";

        // TODO support multiple globals in one line
        for (auto& g : file->Globals) {
            string localName = "0";
            if (g->initVals->size() > 0) {
                localName = compileExpr(os, g->initVals->at(0));
            }
            string varName;
            auto obj = scope->Lookup(g->idents->at(0)).second;
            if (obj != nullptr) {
                varName = obj->MangledName;
            } else {
                cerr << "error: global variable not found" << endl;
                assert(false);
            }
            os << "\tstore i32 " << localName << ", i32* " << varName << "\n";
        }
        os << "\tret i32 0\n";
        os << "}\n";
    }

    void compileFile(ostream& os, CompUnitAST* file) {
        auto re = scope;
        enterScope();

        for (auto& g : file->Globals) {
            stringstream ss;
            ss << "@ugo_" << file->packageName << "_" << g->idents->at(0);
            string mangledName = ss.str();
            scope->Insert(new Object(g->idents->at(0), mangledName, g.get()));
            os << mangledName << " = global i32 0\n";
        }
        if (file->Globals.size() > 0) {
            os << endl;
        }
        for (auto& fn : file->Funcs) {
            compileFunc(os, file, fn.get());
        }
        genInit(os, file);

        restoreScope(re);
    }

    void compileFunc(ostream& os, CompUnitAST* file, FuncDefAST* fn) {
        auto re = scope;
        enterScope();

        stringstream ss;
        ss << "@ugo_" << file->packageName << "_" << fn->ident;
        string mangledName = ss.str();

        scope->Insert(new Object(fn->ident, mangledName, fn));

        // TODO add func decl option in .y and AST
        if (fn->body == nullptr) {
            os << "declare i32 @ugo_" << file->packageName << "_" << fn->ident
               << "()\n";
            return;
        }
        os << endl;

        os << "define i32 @ugo_" << file->packageName << "_" << fn->ident
           << "() {\n";
        compileStmt(os, fn->body);
        os << "\tret i32 0\n";
        os << "}\n";

        restoreScope(re);
    }

    //TODO test scope enter/leave
    void compileStmt(ostream& os, pAST& _stmt) {
        // stmt: *StmtAST
        auto stmt = reinterpret_cast<StmtAST*>(_stmt.get());
        string localName;
        string mangledName;
        string firstId;
        stringstream ss;
        string ifInit, ifCond, ifBody, ifElse, ifEnd;
        string forInit, forCond, forBody, forPost, forEnd;
        static int thePos = 0;
        if (stmt->type() == TType::VarSpecT) {
            // TODO now only support one spec
            auto stmt0 = reinterpret_cast<VarSpecAST*>(stmt);
            localName = "0";
            if (stmt0->initVals->size() > 0) {
                localName = compileExpr(os, (*stmt0->initVals)[0]);
            }
            firstId = (*stmt0->idents)[0];
            ss << "%local_" << firstId << ".pos.0";
            mangledName = ss.str();
            scope->Insert(new Object(firstId, mangledName, stmt0));
            os << "\t" << mangledName << " = alloca i32, align 4\n";
            os << "\tstore i32 " << localName << ", i32* " << mangledName
               << "\n";
        } else if (stmt->type() == TType::ShortVarDeclT) {
            compileStmt_assign(os, _stmt);
        } else if (stmt->type() == TType::IfStmtT) {
            auto stmt1 = reinterpret_cast<IfStmtAST*>(stmt);
            auto ifre1 = scope;
            enterScope();
            ss << thePos++;
            ifInit = genLabelId("if.init.line" + ss.str());
            ifCond = genLabelId("if.cond.line" + ss.str());
            ifBody = genLabelId("if.body.line" + ss.str());
            ifElse = genLabelId("if.else.line" + ss.str());
            ifEnd = genLabelId("if.end.line" + ss.str());
            // br if.init
            os << "\tbr label %" << ifInit << "\n";
            // if.init
            os << "\n" << ifInit << ":\n";
            auto ifre2 = scope;
            enterScope();
            {
                if (stmt1->init != nullptr) {
                    compileStmt(os, stmt1->init);
                    os << "\tbr label %" << ifCond << "\n";
                } else {
                    os << "\tbr label %" << ifCond << "\n";
                }
                // if.cond
                {
                    os << "\n" << ifCond << ":\n";
                    auto cond = compileExpr(os, stmt1->cond);
                    if (stmt1->elseBlockStmt != nullptr) {
                        os << "\tbr i1 " << cond << ", label %" << ifBody
                           << ", label %" << ifElse << "\n";
                    } else {
                        os << "\tbr i1 " << cond << ", label %" << ifBody
                           << ", label %" << ifEnd << "\n";
                    }
                }
                // if.body
                auto ifre3 = scope;
                enterScope();
                {
                    os << "\n" << ifBody << ":\n";
                    compileStmt(os, stmt1->body);
                    if (stmt1->elseBlockStmt != nullptr) {
                        os << "\tbr label %" << ifElse << "\n";
                    } else {
                        os << "\tbr label %" << ifEnd << "\n";
                    }
                }
                restoreScope(ifre3);
                // if.else
                auto ifre4 = scope;
                enterScope();
                {
                    os << "\n" << ifElse << ":\n";
                    if (stmt1->elseBlockStmt != nullptr) {
                        compileStmt(os, stmt1->elseBlockStmt);
                        os << "\tbr label %" << ifEnd << "\n";
                    } else {
                        os << "\tbr label %" << ifEnd << "\n";
                    }
                }
                restoreScope(ifre4);
            }
            restoreScope(ifre2);
            // end
            os << "\n" << ifEnd << ":\n";
            restoreScope(ifre1);
        } else if (stmt->type() == TType::ForStmtT) {
            auto stmt2 = reinterpret_cast<ForStmtAST*>(stmt);
            auto re1 = scope;
            enterScope();
            ss << thePos++;
            forInit = genLabelId("for.init.line" + ss.str());
            forCond = genLabelId("for.cond.line" + ss.str());
            forPost = genLabelId("for.post.line" + ss.str());
            forBody = genLabelId("for.body.line" + ss.str());
            forEnd = genLabelId("for.end.line" + ss.str());
            // br for.init
            os << "\tbr label %" << forInit << "\n";
            auto re2 = scope;
            enterScope();
            {
                os << "\n" << forInit << ":\n";
                if (stmt2->init != nullptr) {
                    compileStmt(os, stmt2->init);
                }
                os << "\tbr label %" << forCond << "\n";

                // for.cond
                os << "\n" << forCond << ":\n";
                if (stmt2->cond != nullptr) {
                    auto cond = compileExpr(os, stmt2->cond);
                    os << "\tbr i1 " << cond << ", label %" << forBody
                       << ", label %" << forEnd << "\n";
                } else {
                    os << "\tbr label %" << forBody << "\n";
                }
                // for.body
                auto re3 = scope;
                enterScope();
                {
                    os << "\n" << forBody << ":\n";
                    compileStmt(os, stmt2->body);
                    os << "\tbr label %" << forPost << "\n";
                }
                restoreScope(re3);
                // for.post
                {
                    os << "\n" << forPost << ":\n";
                    if (stmt2->post != nullptr) {
                        compileStmt(os, stmt2->post);
                    }
                    os << "\tbr label %" << forCond << "\n";
                }
            }
            restoreScope(re2);
            os << "\n" << forEnd << ":\n";
            restoreScope(re1);
        } else if (stmt->type() == TType::BlockT) {
            auto stmt3 = reinterpret_cast<BlockAST*>(stmt);
            auto re = scope;
            enterScope();
            for (auto& stmt : *stmt3->stmts) {
                compileStmt(os, stmt);
            }
            restoreScope(re);
        } else if (stmt->type() == TType::ExpStmtT) {
            auto stmt4 = reinterpret_cast<ExpStmtAST*>(stmt);
            compileExpr(os, stmt4->exp);
        } else {
            cerr << "unknown stmt type" << endl;
            assert(false);
        }
    }

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
            for (auto& target : targets) {
                auto tar = reinterpret_cast<LValAST*>(target.get());
                // TODO support index (tar->indexList)
                if (!scope->HasName(tar->ident)) {
                    stringstream ss;
                    // TODO !!! can we remove pos ??? !!!
                    ss << "%local_" << tar->ident << ".pos.0";
                    auto mangledName = ss.str();
                    scope->Insert(
                        new Object(tar->ident, mangledName, target.get()));
                    os << "\t" << mangledName << " = alloca i32, align 4\n";
                }
            }
        }
        for (int i = 0; i < targets.size(); ++i) {
            auto tar = reinterpret_cast<LValAST*>(targets[i].get());
            string varName = "";
            auto obj = scope->Lookup(tar->ident).second;
            if (obj != nullptr) {
                varName = obj->MangledName;
            } else {
                cerr << "var " << tar->ident << " undefined" << endl;
                assert(false);
            }
            os << "\tstore i32 " << valueNameList[i] << ", i32* " << varName
               << "\n";
        }
    }

    string compileExpr(ostream& os, pAST& _expr) {
        // expr: *ExpAST
        auto expr = reinterpret_cast<ExpAST*>(_expr.get());
        string localName;  // ret
        string varName;
        string funcName;
        if (expr->type() == TType::LValT) {
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
        } else if (expr->type() == TType::NumberT) {
            auto exp1 = reinterpret_cast<NumberAST*>(expr);
            localName = genId();
            os << "\t" << localName << " = "
               << "add"
               << " i32 "
               << "0"
               << ", " << exp1->num << endl;
            return localName;
        } else if (expr->type() == TType::BinExpT) {
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
                    cerr << "compileExpr: unknown type of BinExpAST" << endl;
                    assert(false);
                    break;
            }
        } else if (expr->type() == TType::UnaryExpT) {
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
        } else if (expr->type() == TType::ParenExpT) {
            auto exp4 = reinterpret_cast<ParenExpAST*>(expr);
            return compileExpr(os, exp4->p);
        } else if (expr->type() == TType::CallExpT) {
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
        } else {
            cerr << "compileExpr: unknown type of ExpAST" << endl;
            assert(false);
        }
        return localName;
    }
};