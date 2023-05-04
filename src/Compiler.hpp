#pragma once

#include <AST.hpp>
#include <Scope.hpp>

using namespace std;

class Compiler {
   public:
    CompUnitAST* file;
    Scope* scope;
    // id for generated temp (%t0, %t1, ...) and labels, use with ++
    int nextId;
    // avoid conflict of vars in different scope but with the same name, use with ++
    int varSuffix;
    // suffix for each group of generated labels, use with ++
    int labelSuffix;

    Compiler()
        : scope(Scope::Universe()), nextId(0), varSuffix(0), labelSuffix(0) {}
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
        os << Header;
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

        // register global vars
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
        // register global funcs
        for (auto& fn : file->Funcs) {
            stringstream ss;
            ss << "@ugo_" << file->packageName << "_" << fn->ident;
            auto mangledName = ss.str();
            scope->Insert(new Object(fn->ident, mangledName, fn.get()));
        }
        for (auto& fn : file->Funcs) {
            compileFunc(os, file, fn.get());
        }
        genInit(os, file);

        restoreScope(re);
    }

    void compileFunc(ostream& os, CompUnitAST* file, FuncDefAST* fn) {
        auto paramMNameList = vector<string>();
        for (auto& _param : *fn->paramList) {
            auto param = reinterpret_cast<ParamAST*>(_param.get());
            stringstream ss;
            ss << "%local_" << param->ident << "." << varSuffix++;
            string mangledName = ss.str();
            paramMNameList.push_back(mangledName);
        }

        // TODO add func decl option in .y and AST
        if (fn->body == nullptr) {
            os << "declare i32 @ugo_" << file->packageName << "_" << fn->ident
               << "()\n";
            return;
        }
        os << endl;
        os << "define i32 @ugo_" << file->packageName << "_" << fn->ident
           << "(";
        // params list
        for (int i = 0; i < paramMNameList.size(); i++) {
            os << "i32 " << paramMNameList[i] << ".arg" << i;
            if (i != paramMNameList.size() - 1) {
                os << ", ";
            }
        }
        os << ") {\n";

        // params + body scope
        bool hasRet = false;
        auto re = scope;
        enterScope();
        {
            // stringstream ss;
            // ss << "@ugo_" << file->packageName << "_" << fn->ident;
            // string mangledName = ss.str();
            // scope->Insert(new Object(fn->ident, mangledName, fn));

            // register params
            for (int i = 0; i < fn->paramList->size(); i++) {
                auto param =
                    reinterpret_cast<ParamAST*>(fn->paramList->at(i).get());
                stringstream ss;
                auto mangledName = paramMNameList[i];
                ss << mangledName << ".arg" << i;
                string paramRegName = ss.str();
                scope->Insert(new Object(param->ident, mangledName, fn));

                os << "\t" << mangledName << " = alloca i32, align 4\n";
                os << "\tstore i32 " << paramRegName << ", i32* " << mangledName
                   << "\n";
            }

            // body // TODO test this
            auto body = reinterpret_cast<BlockAST*>(fn->body.get());
            for (auto& stmt : *body->stmts) {
                if (stmt->type() == TType::ReturnStmtT) {
                    hasRet = true;
                }
                compileStmt(os, stmt);
            }
            // ensure ret
            if (!hasRet) {
                os << "\tret i32 0\n";
            }
        }
        restoreScope(re);

        os << "}\n";
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
        if (stmt->type() == TType::VarSpecT) {
            // TODO now only support one spec
            auto stmt0 = reinterpret_cast<VarSpecAST*>(stmt);
            localName = "0";
            if (stmt0->initVals->size() > 0) {
                localName = compileExpr(os, (*stmt0->initVals)[0]);
            }
            firstId = (*stmt0->idents)[0];
            ss << "%local_" << firstId << "." << varSuffix++;
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
            ss << labelSuffix++;
            ifInit = genLabelId("if.init" + ss.str());
            ifCond = genLabelId("if.cond" + ss.str());
            ifBody = genLabelId("if.body" + ss.str());
            ifElse = genLabelId("if.else" + ss.str());
            ifEnd = genLabelId("if.end" + ss.str());
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
            ss << labelSuffix++;
            forInit = genLabelId("for.init" + ss.str());
            forCond = genLabelId("for.cond" + ss.str());
            forPost = genLabelId("for.post" + ss.str());
            forBody = genLabelId("for.body" + ss.str());
            forEnd = genLabelId("for.end" + ss.str());
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
        } else if (stmt->type() == TType::ReturnStmtT) {
            auto stmt5 = reinterpret_cast<ReturnStmtAST*>(stmt);
            if (stmt5->exp != nullptr) {
                auto ret = compileExpr(os, stmt5->exp);
                os << "\tret i32 " << ret << "\n";
            } else {
                // return 0:
                // os << "\tret i32 0\n";
                // return void:
                os << "\tret void\n";
            }
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
                    ss << "%local_" << tar->ident << "." << varSuffix++;
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
            auto left = compileExpr(os, exp2->left);
            auto right = compileExpr(os, exp2->right);
            switch (exp2->op) {
                case BinExpAST::Op::ADD:
                    os << "\t" << localName << " = "
                       << "add"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::SUB:
                    os << "\t" << localName << " = "
                       << "sub"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::MUL:
                    os << "\t" << localName << " = "
                       << "mul"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::DIV:
                    os << "\t" << localName << " = "
                       << "div"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::MOD:
                    os << "\t" << localName << " = "
                       << "srem"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                    // https://llvm.org/docs/LangRef.html#icmp-instruction
                case BinExpAST::Op::EQ:
                    os << "\t" << localName << " = "
                       << "icmp eq"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::NE:
                    os << "\t" << localName << " = "
                       << "icmp ne"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::LT:
                    os << "\t" << localName << " = "
                       << "icmp slt"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::LE:
                    os << "\t" << localName << " = "
                       << "icmp sle"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::GT:
                    os << "\t" << localName << " = "
                       << "icmp sgt"
                       << " i32 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::GE:
                    os << "\t" << localName << " = "
                       << "icmp sge"
                       << " i32 " << left << ", " << right << endl;
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
                auto child = compileExpr(os, exp3->p);
                os << "\t" << localName << " = "
                   << "sub"
                   << " i32 "
                   << "0"
                   << ", " << child << endl;
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
            auto arg0 = compileExpr(os, (*exp5->argList)[0]);
            os << "\t" << localName << " = call i32(i32) " << funcName
               << "(i32 " << arg0 << ")" << endl;
            return localName;
        } else {
            cerr << "compileExpr: unknown type of ExpAST" << endl;
            assert(false);
        }
        return localName;
    }
};