#pragma once

/**
 * @file Compiler.hpp
 * @author Asilvorcarp (asilvorcarp@qq.com)
 * @brief the frontend which compile the AST to LLVM IR
 * @version 2.0
 * @date 2023-06-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <AST.hpp>
#include <Scope.hpp>

using namespace std;

/**
 * @brief the frontend which compile the AST to LLVM IR
 */
class Compiler {
   public:
    /**
     * @brief the CompUnitAST to compile
     */
    CompUnitAST* file;
    /**
     * @brief the current scope
     */
    Scope* scope;
    /**
     * @brief id for generated temp (%t0, %t1, ...) and labels, use with ++
     */
    int nextId = 0;
    /**
     * @brief avoid conflict of vars in different scope but with the same name, use with ++
     */
    int varSuffix = 0;
    /**
     * @brief suffix for each group of generated labels, use with ++
     */
    int labelSuffix = 0;
    /**
     * @brief whether print debug info or not
     */
    bool debug = false;

    /**
     * @brief Construct a new Compiler object with the Universe scope
     */
    Compiler() : scope(Scope::Universe()) {}
    ~Compiler() {}

    /**
     * @brief compile the whole CompUnitAST (the file)
     * 
     * @param _file the CompUnitAST (file) to compile
     * @return string, the compiled LLVM IR code
     */
    string Compile(CompUnitAST* _file) {
        stringstream ss;

        file = _file;

        genHeader(ss, file);
        compileFile(ss, file);
        genMain(ss, file);

        return ss.str();
    }

    /**
     * @brief enter a new scope
     */
    void enterScope() { scope = new Scope(scope); }
    /**
     * @brief leave the current scope to the outer scope
     */
    void leaveScope() { scope = scope->Outer; }
    /**
     * @brief restore the scope to the given scope
     * 
     * @param s the scope to restore to
     */
    void restoreScope(Scope* s) { scope = s; }

    /**
     * @brief generate the header, including the package name and the runtime functions
     * 
     * @param os the ostream to write to
     * @param file the CompUnitAST (file) to compile
     */
    void genHeader(ostream& os, CompUnitAST* file) {
        os << "; package " << file->packageName << endl;
        os << Header;
    }

    /**
     * @brief generate the main function, which calls main_init and main_main
     * 
     * @param os the ostream to write to
     * @param file the CompUnitAST (file) to compile
     */
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

    /**
     * @brief generate a new temp's id of LLVM IR
     * 
     * @return string - the temp's id
     */
    string genId() {
        stringstream ss;
        ss << "%t" << nextId++;
        return ss.str();
    }

    /**
     * @brief generate a new label with unique id
     * 
     * @param name the label's name
     * @return string - the label
     */
    string genLabelId(string name) {
        stringstream ss;
        ss << name << "." << nextId++;
        return ss.str();
    }

    /**
     * @brief generate the LLVM IR of init a var with default val (usually zero val)
     * 
     * @param os the ostream to write to
     * @param varType var's type
     * @param varMName var's mangled name
     */
    void genDefaultInit(ostream& os, string varType, string varMName) {
        if (varType == "i64") {
            // may include i64 in the future
            os << "\tstore " << varType << " 0, " << increaseDim(varType) << " "
               << varMName << "\n";
        } else {
            // TODO maybe init array, ptr
            if (debug) {
                clog << "info: var not inited - " << varMName << endl;
            }
        }
    }

    /**
     * @brief generate the function to init global vars called X_init
     * 
     * @param os the ostream to write to
     * @param file the CompUnitAST (file) to compile
     */
    void genInit(ostream& os, CompUnitAST* file) {
        // the function to init globals
        os << "define void @" << file->packageName << "_init() {\n";

        for (auto& g : file->Globals) {
            int idNum = g->idents->size();
            int valNum = g->initVals->size();
            for (int i = 0; i < idNum; i++) {
                auto obj = scope->Lookup(g->idents->at(i)).second;
                if (obj == nullptr) {
                    cerr << "error: global variable undefined" << endl;
                    assert(false);
                }
                string varType = obj->Node->info();
                string mangledName = obj->MangledName;
                // assert valNum == 0 || valNum == idNum;
                if (valNum != 0 && valNum != idNum) {
                    cerr << "error: global id and init value number not match"
                         << endl;
                    assert(false);
                }
                if (valNum > 0) {
                    // init with val
                    auto valLocal = compileExpr(os, g->initVals->at(i));
                    os << "\tstore " << varType << " " << valLocal << ", "
                       << increaseDim(varType) << " " << mangledName << "\n";
                } else {
                    // init with default val (zero val)
                    genDefaultInit(os, varType, mangledName);
                }
            }
        }
        os << "\tret void\n";
        os << "}\n";
    }

    /**
     * @brief compile the file without the header and fake main function
     * 
     * @param os the ostream to write to
     * @param file the CompUnitAST (file) to compile
     */
    void compileFile(ostream& os, CompUnitAST* file) {
        auto re = scope;
        enterScope();

        // register global vars
        // note: initialized in genInit()
        for (auto& g : file->Globals) {
            auto ast = g.get();  // VarSpecAST
            // get mangled name
            stringstream ss;
            int idNum = ast->idents->size();
            string varType = g->info();
            if (varType == "infer") {
                cerr << "error: global var type not specified" << endl;
                assert(false);
            }
            for (int i = 0; i < idNum; i++) {
                string id = ast->idents->at(i);
                ss << "@" << file->packageName << "_" << id;
                string mangledName = ss.str();
                scope->Insert(new Object(id, mangledName, ast));
                if (isPtr(varType)) {
                    cerr << "error: global var for array not implemented yet"
                         << endl;
                    assert(false);
                    // TODO for array
                    // os << mangledName << " = common global " << varType
                    //    << " zeroinitializer\n";
                } else {
                    os << mangledName << " = global " << varType << " 0\n";
                }
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

    /**
     * @brief compile a function
     * 
     * @param os the ostream to write to
     * @param file the CompUnitAST (file) to compile
     * @param fn the FuncDefAST to compile
     */
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
                   << increaseDim(paramType) << " " << mangledName << "\n";
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
                if (retType == "i64") {
                    os << "\tret i64 0\n";
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

    /**
     * @brief compile a statement
     * 
     * @param os the ostream to write to
     * @param _stmt the StmtAST to compile
     */
    void compileStmt(ostream& os, const pAST& _stmt) {
        // stmt: *StmtAST
        auto stmt = reinterpret_cast<StmtAST*>(_stmt.get());
        if (debug) {
            clog << ">> compileStmt " << *stmt << endl;
        }
        stringstream ss;
        if (stmt->type() == TType::VarSpecT) {
            auto stm = reinterpret_cast<VarSpecAST*>(stmt);
            // assert idNum == valNum or valNum == 0
            int idNum = stm->idents->size();
            int valNum = stm->initVals->size();
            if (idNum != valNum && valNum != 0) {
                cerr
                    << "compileStmt: VarSpec ids and init vals number not match"
                    << endl;
                assert(false);
            }
            for (int i = 0; i < idNum; i++) {
                string id = stm->idents->at(i);
                // get mangled name
                ss << "%local_" << id << "." << varSuffix++;
                auto mangledName = ss.str();
                // get var type
                auto varType = stm->info();
                if (varType == "infer") {
                    varType = inferType(stm->initVals->at(i));
                }
                // insert obj into scope with LValAST node
                auto node = new LValAST(id, varType);
                scope->Insert(new Object(id, mangledName, node));
                // alloc local space to store the var,
                // and mangledName is the ptr to this place
                os << "\t" << mangledName << " = alloca " << varType
                   << ", align 4\n";
                // init
                if (valNum > 0) {
                    // init with val
                    auto valLocal = compileExpr(os, stm->initVals->at(i));
                    os << "\tstore " << varType << " " << valLocal << ", "
                       << increaseDim(varType) << " " << mangledName << "\n";
                } else {
                    // init with default val (zero val)
                    genDefaultInit(os, varType, mangledName);
                }
            }
        } else if (stmt->type() == TType::ShortVarDeclT) {
            compileStmt_assign(os, _stmt);
        } else if (stmt->type() == TType::IfStmtT) {
            auto stmt1 = reinterpret_cast<IfStmtAST*>(stmt);
            // if you are debugging, you can generate more labels
            ss << labelSuffix++;
            // for basic block?
            auto ifInit = genLabelId("if.init" + ss.str());
            auto ifCond = genLabelId("if.cond" + ss.str());
            // the body label is just a placeholder for br
            auto ifBody = genLabelId("if.body" + ss.str());
            // for if-else
            auto ifElse = genLabelId("if.else" + ss.str());
            // for if or if-else
            auto ifEnd = genLabelId("if.end" + ss.str());
            // the scope out of if
            auto outIf = scope;
            // enter scope in if
            // br if.init
            // os << "\tbr label %" << ifInit << "\n";
            // // if.init
            // os << "\n" << ifInit << ":\n";
            enterScope();
            {
                if (stmt1->init != nullptr) {
                    compileStmt(os, stmt1->init);
                }
                // os << "\tbr label %" << ifCond << "\n";
                // // if.cond
                // os << "\n" << ifCond << ":\n";
                auto cond = compileExpr(os, stmt1->cond);
                if (stmt1->elseBlockStmt != nullptr) {
                    os << "\tbr i1 " << cond << ", label %" << ifBody
                       << ", label %" << ifElse << "\n";
                } else {
                    os << "\tbr i1 " << cond << ", label %" << ifBody
                       << ", label %" << ifEnd << "\n";
                }
                // if.body
                auto reScope1 = scope;
                enterScope();
                {
                    os << "\n" << ifBody << ":\n";
                    compileStmt(os, stmt1->body);
                    os << "\tbr label %" << ifEnd << "\n";
                }
                restoreScope(reScope1);
                // if.else
                if (stmt1->elseBlockStmt != nullptr) {
                    auto reScope2 = scope;
                    enterScope();
                    os << "\n" << ifElse << ":\n";
                    compileStmt(os, stmt1->elseBlockStmt);
                    restoreScope(reScope2);
                    os << "\tbr label %" << ifEnd << "\n";
                }
            }
            // end
            os << "\n" << ifEnd << ":\n";
            restoreScope(outIf);
        } else if (stmt->type() == TType::ForStmtT) {
            auto stm = reinterpret_cast<ForStmtAST*>(stmt);
            auto re1 = scope;
            enterScope();
            ss << labelSuffix++;
            auto forCond = stm->getLabel("cond");
            auto forBody = stm->getLabel("body");
            auto forPost = stm->getLabel("post");
            auto forEnd = stm->getLabel("end");
            auto re2 = scope;
            enterScope();
            {
                // os << "\n" << forInit << ":\n";
                if (stm->init != nullptr &&
                    stm->init->type() != TType::EmptyStmtT) {
                    compileStmt(os, stm->init);
                }
                os << "\tbr label %" << forCond << "\n";

                // for.cond
                os << "\n" << forCond << ":\n";
                if (stm->cond != nullptr &&
                    stm->cond->type() != TType::EmptyStmtT) {
                    auto cond = compileExpr(os, stm->cond);
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
                    compileStmt(os, stm->body);
                    os << "\tbr label %" << forPost << "\n";
                }
                restoreScope(re3);
                // for.post
                {
                    os << "\n" << forPost << ":\n";
                    if (stm->post != nullptr &&
                        stm->post->type() != TType::EmptyStmtT) {
                        compileStmt(os, stm->post);
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
            auto stm = reinterpret_cast<ReturnStmtAST*>(stmt);
            if (stm->info() == "void") {
                os << "\tret void\n";
            } else {
                // info() == "infer"
                // TODO hard type check: return type should be the same as function
                auto ret = compileExpr(os, stm->exp);
                auto retType = inferType(stm->exp);
                os << "\tret " << retType << " " << ret << "\n";
            }
        } else if (stmt->type() == TType::BranchStmtT) {
            auto stm = reinterpret_cast<BranchStmtAST*>(stmt);
            string labelSuffix = "";
            string label = "";
            if (stm->t == stm->Break) {
                labelSuffix = "end";
            } else if (stm->t == stm->Continue) {
                labelSuffix = "post";
            } else {
                // goto
                label = stm->ident;
            }
            if (labelSuffix != "") {
                // get the for loop
                pT curr = stmt;
                while (curr->type() != TType::ForStmtT) {
                    curr = curr->getParent();
                }
                auto forStmt = reinterpret_cast<ForStmtAST*>(curr);
                // the label
                label = forStmt->getLabel(labelSuffix);
            }
            os << "\tbr label %" << label << "\n";
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
            if (obj->Node->info() != "i64") {
                // TODO not support a[x]++
                cerr << "IncDecStmt: not support array" << endl;
                assert(false);
            }
            auto val = genId();
            os << "\t" << val << " = load i64, i64* " << varMName
               << ", align 4\n";
            auto newVal = genId();
            os << "\t" << newVal << " = " << op << " i64 " << val << ", "
               << "1" << endl;
            os << "\tstore i64 " << newVal << ", i64* " << varMName
               << ", align 4\n";
        } else {
            cerr << "unknown stmt type" << endl;
            assert(false);
        }
    }

    /**
     * @brief compile a assign statement
     * 
     * @param os the ostream to write to
     * @param _stmt the ShortVarDeclAST to compile
     */
    void compileStmt_assign(ostream& os, const pAST& _stmt) {
        // stmt: *ShortVarDeclAST
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
                // assert type match
                if (!typeMatch(varType, valueTypeList[i])) {
                    cerr << "compileStmt_assign: varType != valueTypeList[i]"
                         << endl;
                    assert(false);
                }
                os << "\tstore " << valueTypeList[i] << " " << valueNameList[i]
                   << ", " << increaseDim(valueTypeList[i]) << " " << varMName
                   << "\n";
            } else {
                // get %arrayidx
                string ptrName = varMName;
                string curType = varType;
                for (int j = 0; j < tar->indexList->size(); ++j) {
                    auto& idxExp = tar->indexList->at(j);
                    auto idxName = compileExpr(os, idxExp);
                    // assert idxExp is int, type checking
                    if (inferType(idxExp) != "i64") {
                        cerr << "compileStmt_assign: index expression is "
                                "not int"
                             << endl;
                        assert(false);
                    }
                    string ptrValName = genId();
                    os << "\t" << ptrValName << " = load " << curType << ", "
                       << increaseDim(curType) << " " << ptrName << ", align 4"
                       << endl;
                    string nextPtrName = genId();
                    string redCurType = reduceDim(curType);
                    os << "\t" << nextPtrName << " = getelementptr inbounds "
                       << redCurType << ", " << curType << " " << ptrValName
                       << ", i64 " << idxName << "\n";
                    ptrName = nextPtrName;
                    curType = redCurType;
                }
                // assert valueType match curType, type checking
                if (!typeMatch(curType, valueTypeList[i])) {
                    cerr << "compileStmt_assign: valueType != curType" << endl;
                    assert(false);
                }
                os << "\tstore " << valueTypeList[i] << " " << valueNameList[i]
                   << ", " << increaseDim(curType) << " " << ptrName << "\n";
            }
        }
    }

    /**
     * @brief compile a expression
     * 
     * @param os the ostream to write to
     * @param _expr the ExpAST to compile
     * @return string 
     */
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
                   << increaseDim(varType) << " " << varMName << ", align 4\n";
            } else {
                // get %arrayidx
                auto ptrName = varMName;
                auto curType = varType;
                for (int j = 0; j < exp->indexList->size(); ++j) {
                    auto& idxExp = exp->indexList->at(j);
                    auto idxName = compileExpr(os, idxExp);
                    // assert idxExp is int, type checking
                    if (inferType(idxExp) != "i64") {
                        cerr << "compileExpr: index expression is not int"
                             << endl;
                        assert(false);
                    }
                    string ptrValName = genId();
                    os << "\t" << ptrValName << " = load " << curType << ", "
                       << increaseDim(curType) << " " << ptrName << ", align 4"
                       << endl;
                    string nextPtrName = genId();
                    string redCurType = reduceDim(curType);
                    os << "\t" << nextPtrName << " = getelementptr inbounds "
                       << redCurType << ", " << curType << " " << ptrValName
                       << ", i64 " << idxName << "\n";
                    ptrName = nextPtrName;
                    curType = redCurType;
                }
                localName = genId();
                os << "\t" << localName << " = load " << curType << ", "
                   << increaseDim(curType) << " " << ptrName << ", align 4\n";
            }
            return localName;
        } else if (expr->type() == TType::NilT) {
            // llvm const
            return "null";
        } else if (expr->type() == TType::NumberT) {
            auto exp = reinterpret_cast<NumberAST*>(expr);
            localName = genId();
            os << "\t" << localName << " = "
               << "add"
               << " i64 "
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
            auto leftType = inferType(exp->left);
            if (debug) {
                clog << ">> doing right" << endl;
            }
            auto right = compileExpr(os, exp->right);
            auto rightType = inferType(exp->right);
            if (debug) {
                clog << ">> done right" << endl;
            }
            // assert one direction of typeMatch is true
            // e.g. i64*<-nil, i64*->nil
            bool isMatch = typeMatch(leftType, rightType) ||
                           typeMatch(rightType, leftType);
            if (!isMatch) {
                cerr << "compileExpr: type mismatch" << endl;
                cerr << " - left: " << leftType << ", right: " << rightType
                     << endl;
                cerr << " - ast: " << *exp << endl;
                assert(false);
            }
            // get type of result
            string finalType = leftType;
            if (isPtr(leftType)) {
                // assert op is EQ or NE
                if (exp->op != BinExpAST::Op::EQ &&
                    exp->op != BinExpAST::Op::NE) {
                    cerr
                        << "compileExpr: pointers can only be compared with EQ "
                           "or NE"
                        << endl;
                    assert(false);
                }
            }
            // result cannot be nil (if both nil), just return true
            if (finalType == "nil") {
                os << "\t" << localName << " = "
                   << "add"
                   << " i64 "
                   << "0"
                   << ", "
                   << "1" << endl;
                return localName;
            }
            switch (exp->op) {
                case BinExpAST::Op::ADD:
                    os << "\t" << localName << " = "
                       << "add"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::SUB:
                    os << "\t" << localName << " = "
                       << "sub"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::MUL:
                    os << "\t" << localName << " = "
                       << "mul"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::DIV:
                    os << "\t" << localName << " = "
                       << "sdiv"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::MOD:
                    os << "\t" << localName << " = "
                       << "srem"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                    // https://llvm.org/docs/LangRef.html#icmp-instruction
                case BinExpAST::Op::EQ:
                    os << "\t" << localName << " = "
                       << "icmp eq"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::NE:
                    os << "\t" << localName << " = "
                       << "icmp ne"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::LT:
                    os << "\t" << localName << " = "
                       << "icmp slt"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::LE:
                    os << "\t" << localName << " = "
                       << "icmp sle"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::GT:
                    os << "\t" << localName << " = "
                       << "icmp sgt"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::GE:
                    os << "\t" << localName << " = "
                       << "icmp sge"
                       << " " << finalType << " " << left << ", " << right
                       << endl;
                    break;
                case BinExpAST::Op::AND:
                    os << "\t" << localName << " = "
                       << "and"
                       << " i1 " << left << ", " << right << endl;
                    break;
                case BinExpAST::Op::OR:
                    os << "\t" << localName << " = "
                       << "or"
                       << " i1 " << left << ", " << right << endl;
                    break;
                default:
                    cerr << "compileExpr: unknown type of BinExpAST" << endl;
                    assert(false);
                    break;
            }
            if (debug) {
                clog << ">> done bin self" << endl;
            }
            return localName;
        } else if (expr->type() == TType::UnaryExpT) {
            auto exp = reinterpret_cast<UnaryExpAST*>(expr);
            if (exp->op == '-') {
                localName = genId();
                auto child = compileExpr(os, exp->p);
                os << "\t" << localName << " = "
                   << "sub"
                   << " i64 "
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
            // assert varType is array
            if (varType == "i64") {
                cerr << "compileExpr: make type is not array" << endl;
                assert(false);
            }
            // assert len is int, maybe error while eval-ing
            if (inferType(exp->len) != "i64") {
                cerr << "compileExpr: make len must be int" << endl;
                assert(false);
            }
            string lenLocal = compileExpr(os, exp->len);
            auto localName = genId();
            {  // alloca on stack
                // os << "\t" << localName << " = "
                //    << "alloca " << varType << ", i64 " << lenLocal
                //    << ", align 4" << endl;
            }
            // alloca on heap, call malloc
            // get size of element type
            auto elemType = reduceDim(varType);
            auto elemSize = 8;  // ptr in x86_64 machine
            if (elemType == "i64") {
                elemSize = 8;  // x64
            } else if (elemType == "i32") {
                elemSize = 4;  // int
            }
            // get size of array
            auto sizeLocal = genId();
            os << "\t" << sizeLocal << " = "
               << "mul"
               << " i64 " << lenLocal << ", " << to_string(elemSize) << endl;
            // call malloc which returns i8*
            auto i8pName = genId();
            auto i8pType = "i8*";
            os << "\t" << i8pName << " = "
               << "call noalias " << i8pType << " @malloc(i64 " << sizeLocal
               << ")" << endl;
            // convert the ptr type with bitcast
            os << "\t" << localName << " = "
               << "bitcast " << i8pType << " " << i8pName << " to " << varType
               << endl;
            return localName;
        } else if (expr->type() == TType::ArrayExpT) {
            auto exp = reinterpret_cast<ArrayExpAST*>(expr);
            auto varType = exp->info();
            // assert varType is array
            if (varType == "i64") {
                cerr << "compileExpr: array exp type is not array" << endl;
                assert(false);
            }
            auto localName = genId();
            {  // alloca on stack
                // os << "\t" << localName << " = "
                //    << "alloca " << varType << ", i64 " << elemNum
                //    << ", align 4" << endl;
            }
            // alloca on heap, call malloc
            // get size of element type
            auto elemType = reduceDim(varType);
            auto elemSize = 8;  // ptr in x86_64 machine
            if (elemType == "i64") {
                elemSize = 8;  // x64
            } else if (elemType == "i32") {
                elemSize = 4;  // int
            }
            // get size of array
            int elemNum = exp->initValList->size();
            auto arrSize = elemNum * elemSize;
            // call malloc which returns i8*
            auto i8pName = genId();
            auto i8pType = "i8*";
            os << "\t" << i8pName << " = "
               << "call noalias " << i8pType << " @malloc(i64 "
               << to_string(arrSize) << ")" << endl;
            // convert the ptr type with bitcast
            os << "\t" << localName << " = "
               << "bitcast " << i8pType << " " << i8pName << " to " << varType
               << endl;
            // get and store each element
            for (int i = 0; i < elemNum; i++) {
                auto initValLocal = compileExpr(os, exp->initValList->at(i));
                auto initValType = inferType(exp->initValList->at(i));
                // check type
                if (!typeMatch(elemType, initValType)) {
                    cerr << "compileExpr: array exp type mismatch" << endl;
                    assert(false);
                }
                auto idxLocal = to_string(i);
                auto elemPtrLocal = genId();
                // here varType is usually elemType*
                os << "\t" << elemPtrLocal << " = "
                   << "getelementptr inbounds " << elemType << ", " << varType
                   << " " << localName << ", i64 " << idxLocal << endl;
                os << "\t"
                   << "store " << elemType << " " << initValLocal << ", "
                   << increaseDim(elemType) << " " << elemPtrLocal << endl;
            }
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
            auto funcDefAST = reinterpret_cast<FuncDefAST*>(obj->Node);
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
                sub << localName;
            } else {
                localName = genId();
                sub << localName << " = ";
            }
            os << "\t" << sub.str() << "call " << funcType << " " << funcName
               << "(";
            for (int i = 0; i < argNum; i++) {
                // type check for param and arg
                if (!typeMatch(paramTypes->at(i), argTypes[i])) {
                    cerr << "compileExpr: type mismatch in function call - "
                         << funcName << endl;
                    // show the arg type and param type
                    cerr << " - arg has type \"" << argTypes[i]
                         << "\", but expected \"" << paramTypes->at(i) << "\"."
                         << endl;
                    cerr << " - arg AST: " << *exp->argList->at(i) << endl;
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

    /**
     * @brief reduce dimension of array type
     * 
     * @param t the type string
     * @return string - the reduced type string
     * 
     * @note change "[5 x [4 x i64]]" to "[4 x i64]"
     * @note change "i64**" to "i64*"
     */
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

    /**
     * @brief reduce dimension of array type for dim times
     * 
     * @param t the type string
     * @param dim the number of times to reduce
     * @return string - the reduced type string
     */
    string reduceDim(string t, int dim) {
        for (int i = 0; i < dim; i++) {
            t = reduceDim(t);
        }
        return t;
    }

    /**
     * @brief increase dimension of array type
     * @note support only pointer now (not like [5 x [4 x i64]])
     * 
     * @param t 
     * @return string 
     */
    string increaseDim(string t) { return t + "*"; }

    /**
     * @brief whether a type is a pointer type
     * 
     * @param t the type string
     * @return true 
     * @return false 
     */
    bool isPtr(string t) { return t.find("*") != string::npos || t == "nil"; }

    /**
     * @brief try to fit tRight to tLeft, if cannot, return false
     * @note tRight maybe change to tLeft if it is nil
     * 
     * @param tLeft the type string of LHS
     * @param tRight the type string of RHS
     * @return true 
     * @return false 
     */
    bool typeMatch(string tLeft, string& tRight) {
        // TODO use this in all type checking system
        if (tLeft == tRight) return true;
        // right side is nil, left is X*
        // convert nil to X*, and return true
        if (tRight == "nil" && isPtr(tLeft)) {
            tRight = tLeft;
            return true;
        }
        return false;
    }

    /**
     * @brief infer the type of an expression
     * 
     * @param _expr the expression AST
     * @return string 
     */
    string inferType(const pAST& _expr) {
        auto expr = reinterpret_cast<ExpAST*>(_expr.get());
        if (expr->type() == TType::NumberT) {
            return "i64";
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
                auto funcDefAST = reinterpret_cast<FuncDefAST*>(obj->Node);
                return funcDefAST->getRetType();
            } else {
                cerr << "inferType: function " << exp->funcName << " undefined"
                     << endl;
                assert(false);
                return "";
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
        } else if (expr->type() == TType::ArrayExpT) {
            // always specified in code, like []int{1, 2, 3}
            return expr->info();
        } else if (expr->type() == TType::NilT) {
            return expr->info();
        } else {
            cerr << "inferType: unknown type of ExpAST" << endl;
            assert(false);
            return "";
        }
        return "";
    }
};