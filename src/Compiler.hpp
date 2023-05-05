#pragma once

#include <AST.hpp>
#include <Scope.hpp>

using namespace std;

class Compiler {
   public:
    CompUnitAST* file;
    Scope* scope;
    // id for generated temp (%t0, %t1, ...) and labels, use with ++
    int nextId = 0;
    // avoid conflict of vars in different scope but with the same name, use with ++
    int varSuffix = 0;
    // suffix for each group of generated labels, use with ++
    int labelSuffix = 0;
    // whether print debug info or not
    bool debug = false;

    Compiler() : scope(Scope::Universe()) {}
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

    void genDefaultInit(ostream& os, string varType, string varMName) {
        if (varType == "i32") {
            os << "\tstore " << varType << " 0, " << varType << "* " << varMName
               << "\n";
        } else {
            // TODO maybe init array, ptr
            if (debug) {
                cerr << "info: global var not inited - " << varMName << endl;
            }
        }
    }

    void genInit(ostream& os, CompUnitAST* file) {
        os << "define void @" << file->packageName << "_init() {\n";

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
                cerr << "error: global variable undefined" << endl;
                assert(false);
            }
            auto varType = obj->Node->info();
            if (localName == "0") {
                genDefaultInit(os, varType, varName);
            }
        }
        os << "\tret void\n";
        os << "}\n";
    }

    void compileFile(ostream& os, CompUnitAST* file) {
        auto re = scope;
        enterScope();

        // register global vars
        for (auto& g : file->Globals) {
            stringstream ss;
            ss << "@" << file->packageName << "_" << g->idents->at(0);
            string mangledName = ss.str();
            string varType = g->info();
            scope->Insert(new Object(g->idents->at(0), mangledName, g.get()));
            if (varType == "i32") {
                os << mangledName << " = global " << varType << " 0\n";
            } else {
                // TODO test this
                os << mangledName << " = common global " << varType
                   << " zeroinitializer\n";
            }
        }
        if (file->Globals.size() > 0) {
            os << endl;
        }
        // register global funcs
        for (auto& fn : file->Funcs) {
            stringstream ss;
            ss << "@" << file->packageName << "_" << fn->ident;
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
        auto retType = fn->getRetType();
        auto paramTypes = fn->getParamTypes();
        for (auto& _param : *fn->paramList) {
            auto param = reinterpret_cast<ParamAST*>(_param.get());
            stringstream ss;
            ss << "%local_" << param->ident << "." << varSuffix++;
            string mangledName = ss.str();
            paramMNameList.push_back(mangledName);
        }

        // TODO add func decl option in .y and AST
        if (fn->body == nullptr) {
            os << "declare " << retType << " @" << file->packageName << "_"
               << fn->ident << "()\n";
            return;
        }
        os << endl;
        os << "define " << retType << " @" << file->packageName << "_"
           << fn->ident << "(";
        // params list
        for (int i = 0; i < paramMNameList.size(); i++) {
            os << paramTypes->at(i) << " " << paramMNameList[i] << ".arg" << i;
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
            // register params
            for (int i = 0; i < fn->paramList->size(); i++) {
                auto param =
                    reinterpret_cast<ParamAST*>(fn->paramList->at(i).get());
                auto paramType = param->info();
                stringstream ss;
                auto mangledName = paramMNameList[i];
                ss << mangledName << ".arg" << i;
                string inputArgName = ss.str();
                scope->Insert(new Object(param->ident, mangledName, param));

                os << "\t" << mangledName << " = alloca " << paramType
                   << ", align 4\n";
                os << "\tstore " << paramType << " " << inputArgName << ", "
                   << paramType << "* " << mangledName << "\n";
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
            auto retType = fn->getRetType();
            if (!hasRet) {
                if (retType == "i32") {
                    os << "\tret i32 0\n";
                } else if (retType == "void") {
                    os << "\tret void\n";
                } else {
                    cerr << "error: lack of ret in func " << fn->ident << endl;
                    assert(false);
                }
            }
        }
        restoreScope(re);

        os << "}\n";
    }

    void compileStmt(ostream& os, const pAST& _stmt) {
        // stmt: *StmtAST
        auto stmt = reinterpret_cast<StmtAST*>(_stmt.get());
        if (debug) {
            clog << ">> compileStmt " << *stmt << endl;
        }
        stringstream ss;
        if (stmt->type() == TType::VarSpecT) {
            // TODO now only support one spec
            auto stmt0 = reinterpret_cast<VarSpecAST*>(stmt);
            string localName = "0";  // default init val
            if (stmt0->initVals->size() > 0) {
                localName = compileExpr(os, (*stmt0->initVals)[0]);
            }
            auto firstId = (*stmt0->idents)[0];
            ss << "%local_" << firstId << "." << varSuffix++;
            auto mangledName = ss.str();
            scope->Insert(new Object(firstId, mangledName, stmt0));
            auto varType = stmt0->info();
            os << "\t" << mangledName << " = alloca " << varType
               << ", align 4\n";
            if (localName == "0") {
                genDefaultInit(os, varType, mangledName);
            }
        } else if (stmt->type() == TType::ShortVarDeclT) {
            compileStmt_assign(os, _stmt);
        } else if (stmt->type() == TType::IfStmtT) {
            auto stmt1 = reinterpret_cast<IfStmtAST*>(stmt);
            auto ifre1 = scope;
            enterScope();
            ss << labelSuffix++;
            auto ifInit = genLabelId("if.init" + ss.str());
            auto ifCond = genLabelId("if.cond" + ss.str());
            auto ifBody = genLabelId("if.body" + ss.str());
            auto ifElse = genLabelId("if.else" + ss.str());
            auto ifEnd = genLabelId("if.end" + ss.str());
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
            auto forInit = genLabelId("for.init" + ss.str());
            auto forCond = genLabelId("for.cond" + ss.str());
            auto forPost = genLabelId("for.post" + ss.str());
            auto forBody = genLabelId("for.body" + ss.str());
            auto forEnd = genLabelId("for.end" + ss.str());
            // br for.init
            os << "\tbr label %" << forInit << "\n";
            auto re2 = scope;
            enterScope();
            {
                os << "\n" << forInit << ":\n";
                if (stmt2->init != nullptr &&
                    stmt2->init->type() != TType::EmptyStmtT) {
                    compileStmt(os, stmt2->init);
                }
                os << "\tbr label %" << forCond << "\n";

                // for.cond
                os << "\n" << forCond << ":\n";
                if (stmt2->cond != nullptr &&
                    stmt2->cond->type() != TType::EmptyStmtT) {
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
                    if (stmt2->post != nullptr &&
                        stmt2->post->type() != TType::EmptyStmtT) {
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
            // TODO now only support return int or void
            if (stmt5->exp != nullptr) {
                auto ret = compileExpr(os, stmt5->exp);
                os << "\tret i32 " << ret << "\n";
            } else {
                // return 0:
                // os << "\tret i32 0\n";
                // return void:
                os << "\tret void\n";
            }
        } else if (stmt->type() == TType::BranchStmtT) {
            // TODO
            cerr << "TODO support BranchStmtAST" << endl;
            assert(false);
        } else if (stmt->type() == TType::EmptyStmtT) {
            // do nothing
        } else if (stmt->type() == TType::IncDecStmtT) {
            auto stmt6 = reinterpret_cast<IncDecStmtAST*>(stmt);
            auto tar = reinterpret_cast<LValAST*>(stmt6->target.get());
            auto obj = scope->Lookup(tar->ident).second;
            string op = stmt6->isInc ? "add" : "sub";
            if (obj == nullptr) {
                cerr << "IncDecStmt: undefined variable: " << tar->ident
                     << endl;
                assert(false);
            }
            auto varMName = obj->MangledName;
            if (obj->Node->info() != "i32") {
                // TODO not support a[x]++
                cerr << "IncDecStmt: not support array" << endl;
                assert(false);
            }
            auto val = genId();
            os << "\t" << val << " = load i32, i32* " << varMName
               << ", align 4\n";
            auto newVal = genId();
            os << "\t" << newVal << " = " << op << " i32 " << val << ", "
               << "1" << endl;
            os << "\tstore i32 " << newVal << ", i32* " << varMName
               << ", align 4\n";
        } else {
            cerr << "unknown stmt type" << endl;
            assert(false);
        }
    }

    void compileStmt_assign(ostream& os, const pAST& _stmt) {
        // stmt: *ShortVarDeclAST
        // TODO, for now, any short var decl is int, not support array
        auto stmt = reinterpret_cast<ShortVarDeclAST*>(_stmt.get());
        auto& targets = *stmt->targets;
        auto& initVals = *stmt->initVals;
        vector<string> valueNameList(initVals.size());
        vector<string> valueTypeList(initVals.size());
        for (int i = 0; i < (*stmt->targets).size(); ++i) {
            valueNameList[i] = compileExpr(os, initVals[i]);
            valueTypeList[i] = inferType(initVals[i]);
        }
        if (stmt->isDefine == true) {
            for (int i = 0; i < targets.size(); ++i) {
                auto tar = reinterpret_cast<LValAST*>(targets[i].get());
                if (!scope->HasName(tar->ident)) {
                    stringstream ss;
                    ss << "%local_" << tar->ident << "." << varSuffix++;
                    auto mangledName = ss.str();
                    auto varType = inferType(initVals[i]);
                    // give the inserted node (LValAST) info of its type
                    tar->typeInfo = varType;
                    scope->Insert(new Object(tar->ident, mangledName, tar));
                    os << "\t" << mangledName << " = alloca " << varType
                       << ", align 4\n";
                }
            }
        }
        for (int i = 0; i < targets.size(); ++i) {
            auto tar = reinterpret_cast<LValAST*>(targets[i].get());
            string varMName = "";
            auto obj = scope->Lookup(tar->ident).second;
            if (obj != nullptr) {
                varMName = obj->MangledName;
            } else {
                cerr << "var " << tar->ident << " undefined" << endl;
                assert(false);
            }
            auto varType = obj->Node->info();
            // to support index (tar->indexList)
            if (tar->indexList == nullptr) {
                cerr << "compileStmt_assign: indexList is null" << endl;
                assert(false);
            } else if (tar->indexList->empty()) {
                // assert varType == valueTypeList[i]
                if (varType != valueTypeList[i]) {
                    cerr << "compileStmt_assign: varType != valueTypeList[i]"
                         << endl;
                    assert(false);
                }
                os << "\tstore " << valueTypeList[i] << " " << valueNameList[i]
                   << ", " << valueTypeList[i] << "* " << varMName << "\n";
            } else {
                // get %arrayidx
                string ptrName = varMName;
                string curType = varType;
                for (int j = 0; j < tar->indexList->size(); ++j) {
                    auto& idxExp = tar->indexList->at(j);
                    auto idxName = compileExpr(os, idxExp);
                    // assert idxExp is int, type checking
                    if (inferType(idxExp) != "i32") {
                        cerr
                            << "compileStmt_assign: index expression is not int"
                            << endl;
                        assert(false);
                    }
                    string ptrValName = genId();
                    os << "\t" << ptrValName << " = load " << curType << ", "
                       << curType << "* " << ptrName << ", align 4" << endl;
                    string nextPtrName = genId();
                    string redCurType = reduceDim(curType);
                    os << "\t" << nextPtrName << " = getelementptr inbounds "
                       << redCurType << ", " << curType << " " << ptrValName
                       << ", i32 " << idxName << "\n";
                    ptrName = nextPtrName;
                    curType = redCurType;
                }
                // TODO assert valueType == curType, type checking
                os << "\tstore " << valueTypeList[i] << " " << valueNameList[i]
                   << ", " << valueTypeList[i] << "* " << ptrName << "\n";
            }
        }
    }

    string compileExpr(ostream& os, const pAST& _expr) {
        // expr: *ExpAST
        auto expr = reinterpret_cast<ExpAST*>(_expr.get());
        if (debug) {
            clog << ">> compileExpr " << *expr << endl;
        }
        string localName;  // ret
        if (expr->type() == TType::LValT) {
            auto exp = reinterpret_cast<LValAST*>(expr);
            auto obj = scope->Lookup(exp->ident).second;
            if (obj == nullptr) {
                cerr << "var " << exp->ident << " undefined" << endl;
                assert(false);
            }
            auto varMName = obj->MangledName;
            auto varType = obj->Node->info();
            // to support index (exp->indexList)
            if (exp->indexList == nullptr) {
                cerr << "compileExpr: indexList is null" << endl;
                assert(false);
            } else if (exp->indexList->empty()) {
                localName = genId();
                os << "\t" << localName << " = load " << varType << ", "
                   << varType << "* " << varMName << ", align 4\n";
            } else {
                // get %arrayidx
                auto ptrName = varMName;
                auto curType = varType;
                for (int j = 0; j < exp->indexList->size(); ++j) {
                    auto& idxExp = exp->indexList->at(j);
                    auto idxName = compileExpr(os, idxExp);
                    // assert idxExp is int, type checking
                    if (inferType(idxExp) != "i32") {
                        cerr << "compileExpr: index expression is not int"
                             << endl;
                        assert(false);
                    }
                    string ptrValName = genId();
                    os << "\t" << ptrValName << " = load " << curType << ", "
                       << curType << "* " << ptrName << ", align 4" << endl;
                    string nextPtrName = genId();
                    string redCurType = reduceDim(curType);
                    os << "\t" << nextPtrName << " = getelementptr inbounds "
                       << redCurType << ", " << curType << " " << ptrValName
                       << ", i32 " << idxName << "\n";
                    ptrName = nextPtrName;
                    curType = redCurType;
                }
                localName = genId();
                os << "\t" << localName << " = load " << curType << ", "
                   << curType << "* " << ptrName << ", align 4\n";
            }
            return localName;
        } else if (expr->type() == TType::NumberT) {
            auto exp = reinterpret_cast<NumberAST*>(expr);
            localName = genId();
            os << "\t" << localName << " = "
               << "add"
               << " i32 "
               << "0"
               << ", " << exp->num << endl;
            return localName;
        } else if (expr->type() == TType::BinExpT) {
            auto exp = reinterpret_cast<BinExpAST*>(expr);
            localName = genId();
            if (debug) {
                clog << ">> doing left" << endl;
            }
            auto left = compileExpr(os, exp->left);
            if (debug) {
                clog << ">> doing right" << endl;
            }
            auto right = compileExpr(os, exp->right);
            if (debug) {
                clog << ">> done right" << endl;
            }
            switch (exp->op) {
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
                case BinExpAST::Op::AND:
                    os << "\t" << localName << " = "
                       << "and"
                       << " i1 " << left << ", " << right << endl;
                    return localName;
                    break;
                case BinExpAST::Op::OR:
                    os << "\t" << localName << " = "
                       << "or"
                       << " i1 " << left << ", " << right << endl;
                    return localName;
                    break;
                default:
                    cerr << "compileExpr: unknown type of BinExpAST" << endl;
                    assert(false);
                    break;
            }
            if (debug) {
                clog << ">> done bin self" << endl;
            }
        } else if (expr->type() == TType::UnaryExpT) {
            auto exp = reinterpret_cast<UnaryExpAST*>(expr);
            if (exp->op == '-') {
                localName = genId();
                auto child = compileExpr(os, exp->p);
                os << "\t" << localName << " = "
                   << "sub"
                   << " i32 "
                   << "0"
                   << ", " << child << endl;
                return localName;
            }
            return compileExpr(os, exp->p);
        } else if (expr->type() == TType::ParenExpT) {
            auto exp = reinterpret_cast<ParenExpAST*>(expr);
            return compileExpr(os, exp->p);
        } else if (expr->type() == TType::MakeExpT) {
            auto exp = reinterpret_cast<MakeExpAST*>(expr);
            auto varType = exp->info();
            // assert len is int, maybe error while eval-ing
            if (inferType(exp->len) != "i32") {
                cerr << "compileExpr: make len must be int" << endl;
                assert(false);
            }
            string lenLocal = compileExpr(os, exp->len);
            auto localName = genId();
            os << "\t" << localName << " = "
               << "alloca " << varType << ", i32 " << lenLocal << ", align 4"
               << endl;
            return localName;
        } else if (expr->type() == TType::CallExpT) {
            auto exp = reinterpret_cast<CallExpAST*>(expr);
            auto obj = scope->Lookup(exp->funcName).second;
            string funcName;
            if (obj != nullptr) {
                funcName = obj->MangledName;
            } else {
                cerr << "compileExpr: function " << exp->funcName
                     << " undefined" << endl;
                assert(false);
            }
            auto funcDefAST = dynamic_cast<FuncDefAST*>(obj->Node);
            int argNum = exp->argList->size();
            // get return type and param types
            auto paramTypes = funcDefAST->getParamTypes();
            string funcType = funcDefAST->info();
            // args
            vector<string> argNames;
            vector<string> argTypes;
            for (int i = 0; i < argNum; i++) {
                auto argName = compileExpr(os, exp->argList->at(i));
                argNames.push_back(argName);
                auto argType = inferType(exp->argList->at(i));
                argTypes.push_back(argType);
            }
            // no localName if return void
            auto returnType = funcDefAST->getRetType();
            stringstream sub;
            if (returnType == "void") {
                localName = "";
            } else {
                localName = genId();
                sub << localName << " = ";
            }
            os << "\t" << sub.str() << "call " << funcType << " " << funcName
               << "(";
            for (int i = 0; i < argNum; i++) {
                // type check for arg and param
                if (argTypes[i] != paramTypes->at(i)) {
                    cerr << "compileExpr: type mismatch in function call - "
                         << funcName << endl;
                    // show the arg type and param type
                    cerr << " - arg \"" << argNames[i] << "\" has type \""
                         << argTypes[i] << "\", but expected \""
                         << paramTypes->at(i) << "\"." << endl;
                    assert(false);
                }
                os << paramTypes->at(i) << " " << argNames[i];
                if (i != argNum - 1) os << ", ";
            }
            os << ")" << endl;
            return localName;
        } else {
            cerr << "compileExpr: unknown type of ExpAST" << endl;
            assert(false);
        }
        if (debug) {
            clog << ">> compileExpr: done, localName: " << localName << endl;
        }
        return localName;
    }

    // TODO test this
    // change "[5 x [4 x i32]]" to "[4 x i32]"
    // change "i32**" to "i32*"
    string reduceDim(string t) {
        int i = 0;
        while (i < t.size() && t[i] != 'x') i++;
        if (i == t.size()) {
            if (t.find("*") != string::npos) {
                return t.substr(0, t.size() - 1);
            } else {
                cerr << "reduceDim: not an array type" << endl;
                assert(false);
            }
        }
        int j = t.size() - 1;
        while (j >= 0 && t[j] != ']') j--;
        if (j == -1) {
            if (t.find("*") != string::npos) {
                return t.substr(0, t.size() - 1);
            } else {
                cerr << "reduceDim: not an array type" << endl;
                assert(false);
            }
        }
        string res = t.substr(i + 2, j - i - 2);
        return res;
    }

    string reduceDim(string t, int dim) {
        for (int i = 0; i < dim; i++) {
            t = reduceDim(t);
        }
        return t;
    }

    string inferType(const pAST& _expr) {
        auto expr = reinterpret_cast<ExpAST*>(_expr.get());
        if (expr->type() == TType::NumberT) {
            return "i32";
        } else if (expr->type() == TType::BinExpT) {
            auto exp = reinterpret_cast<BinExpAST*>(expr);
            auto left = inferType(exp->left);
            auto right = inferType(exp->right);
            if (left != right) {
                cerr << "inferType: type mismatch" << endl;
                assert(false);
            }
            return left;
        } else if (expr->type() == TType::UnaryExpT) {
            auto exp = reinterpret_cast<UnaryExpAST*>(expr);
            return inferType(exp->p);
        } else if (expr->type() == TType::ParenExpT) {
            auto exp = reinterpret_cast<ParenExpAST*>(expr);
            return inferType(exp->p);
        } else if (expr->type() == TType::CallExpT) {
            auto exp = reinterpret_cast<CallExpAST*>(expr);
            auto obj = scope->Lookup(exp->funcName).second;
            if (obj != nullptr) {
                auto funcDefAST = dynamic_cast<FuncDefAST*>(obj->Node);
                return funcDefAST->getRetType();
            } else {
                cerr << "inferType: function " << exp->funcName << " undefined"
                     << endl;
                assert(false);
            }
        } else if (expr->type() == TType::LValT) {
            auto exp = reinterpret_cast<LValAST*>(expr);
            auto obj = scope->Lookup(exp->ident).second;
            if (obj == nullptr) {
                cerr << "inferType: variable " << exp->ident << " undefined"
                     << endl;
                assert(false);
            }
            auto baseType = obj->Node->info();
            if (exp->indexList == nullptr) {
                cerr << "inferType: indexList is null" << endl;
                assert(false);
            }
            return reduceDim(baseType, exp->indexList->size());
        } else if (expr->type() == TType::MakeExpT) {
            return expr->info();
        } else {
            cerr << "inferType: unknown type of ExpAST" << endl;
            assert(false);
        }
    }
};