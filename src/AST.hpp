#pragma once

#include <bits/stdc++.h>

#include <iostream>
#include <json.hpp>
#include <memory>
#include <string>

using json = nlohmann::json;
using namespace std;

enum class TType {
    CompUnitT,
    FuncDefT,
    ParamT,
    BlockT,
    BTypeT,
    // StmtT,
    VarSpecT,
    EmptyStmtT,
    ExpStmtT,
    ReturnStmtT,
    ForStmtT,
    BranchStmtT,
    IncDecStmtT,
    ShortVarDeclT,
    // AssignStmtT,
    IfStmtT,
    // ExpT,
    LValT,
    NumberT,
    BinExpT,
    UnaryExpT,
    ParenExpT,
    CallExpT,
    // for future use
    RuntimeFuncT,
    _1,
    _2,
    _3
};

// 所有 AST 的基类
class BaseAST {
   public:
    virtual ~BaseAST() = default;
    virtual TType type() const = 0;
    virtual json toJson() const = 0;
    friend ostream &operator<<(ostream &os, const BaseAST &ast) {
        return os << ast.toJson().dump(4);
    }
    virtual void add(BaseAST *ast){};
    // some of AST implement this to get type info
    virtual string info() const { return "base"; }
};

using pAST = unique_ptr<BaseAST>;
using vpAST = vector<pAST>;
using pvpAST = unique_ptr<vpAST>;
using pvStr = unique_ptr<vector<string>>;

class StmtAST : public BaseAST {};
class ExpAST : public BaseAST {
   public:
    virtual int eval() const = 0;
};

class VarSpecAST : public BaseAST {
   public:
    TType ty = TType::VarSpecT;
    pvStr idents;
    pAST btype = nullptr;
    pvpAST initVals = make_unique<vpAST>();

    virtual string info() const override {
        if (btype == nullptr) return "i32";
        return btype->info();
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "VarSpecAST";
        j["idents"] = json::array();
        for (auto &ident : *idents) {
            j["idents"].push_back(ident);
        }
        if (btype != nullptr) {
            j["btype"] = btype->toJson();
        }
        j["initVals"] = json::array();
        for (auto &initVal : *initVals) {
            j["initVals"].push_back(initVal->toJson());
        }
        return j;
    }
};

class FuncDefAST : public BaseAST {
   public:
    TType ty = TType::FuncDefT;
    string ident;
    pvpAST paramList;
    pAST retType;  // BType
    pAST body;

    virtual string getRetType() {
        if (retType == nullptr) {
            cerr << "retType is nullptr" << endl;
            assert(false);
        }
        return retType->info();
    }
    virtual vector<string> *getParamTypes() {
        auto ret = new vector<string>();
        for (auto &param : *paramList) {
            ret->push_back(param->info());
        }
        return ret;
    }
    virtual string info() const override {
        string ret = retType->info() + "(";
        for (int i = 0; i < paramList->size(); i++) {
            ret += paramList->at(i)->info();
            if (i != paramList->size() - 1) ret += ", ";
        }
        ret += ")";
        return ret;
    }
    virtual TType type() const override { return ty; }
    virtual json toJson() const override {
        json j;
        j["type"] = "FuncDefAST";
        j["ident"] = ident;
        j["paramList"] = json::array();
        for (auto &param : *paramList) {
            j["paramList"].push_back(param->toJson());
        }
        j["retType"] = retType->toJson();
        j["body"] = body->toJson();
        return j;
    }
};

class RuntimeFuncAST : public FuncDefAST {
   public:
    TType ty = TType::RuntimeFuncT;
    string llRetType;
    pvStr paramTypes;

    RuntimeFuncAST(string llRetType, vector<string> *paramTypes)
        : llRetType(llRetType), paramTypes(pvStr(paramTypes)) {
        this->ident = "some_runtime_func";
    }

    string getRetType() override { return llRetType; }
    vector<string> *getParamTypes() override { return paramTypes.get(); }
    string info() const override {
        string ret = llRetType + "(";
        for (int i = 0; i < paramTypes->size(); i++) {
            ret += paramTypes->at(i);
            if (i != paramTypes->size() - 1) ret += ", ";
        }
        ret += ")";
        return ret;
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "RuntimeFuncAST";
        j["llRetType"] = llRetType;
        j["paramTypes"] = json::array();
        for (auto &paramType : *paramTypes) {
            j["paramTypes"].push_back(paramType);
        }
        return j;
    }
};

class CompUnitAST : public BaseAST {
   public:
    TType ty = TType::CompUnitT;
    string packageName;
    vector<unique_ptr<VarSpecAST>> Globals;
    vector<unique_ptr<FuncDefAST>> Funcs;

    void setDefs(vpAST *topDeclList) {
        for (auto &topDef : *topDeclList) {
            if (topDef->type() == TType::VarSpecT) {
                Globals.push_back(
                    unique_ptr<VarSpecAST>((VarSpecAST *)topDef.get()));
            } else if (topDef->type() == TType::FuncDefT) {
                Funcs.push_back(
                    unique_ptr<FuncDefAST>((FuncDefAST *)topDef.get()));
            }
        }
    }

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "CompUnitAST";
        j["packageName"] = packageName;
        j["Globals"] = json::array();
        for (auto &g : Globals) {
            j["Globals"].push_back(g->toJson());
        }
        j["Funcs"] = json::array();
        for (auto &f : Funcs) {
            j["Funcs"].push_back(f->toJson());
        }
        return j;
    }
};

class ParamAST : public BaseAST {
   public:
    TType ty = TType::ParamT;
    string ident;
    pAST t;  // BType

    string info() const override { return t->info(); }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ParamAST";
        j["ident"] = ident;
        j["t"] = t->toJson();
        return j;
    }
};

// class ReturnTypeAST : public BaseAST {
//    public:
//     TType ty = TType::ReturnRypeT;
//     string t = "void";  // default void

//     string info() const override { return t; }
//     TType type() const override { return ty; }
//     json toJson() const override {
//         json j;
//         j["type"] = "ReturnTypeAST";
//         j["t"] = t;
//         return j;
//     }
// };

class BlockAST : public BaseAST {
   public:
    TType ty = TType::BlockT;
    pvpAST stmts;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BlockAST";
        j["stmts"] = json::array();
        for (auto &stmt : *stmts) {
            j["stmts"].push_back(stmt->toJson());
        }
        return j;
    }
};

class EmptyStmtAST : public StmtAST {
   public:
    TType ty = TType::EmptyStmtT;
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "EmptyStmtAST";
        return j;
    }
};

class ReturnStmtAST : public StmtAST {
   public:
    TType ty = TType::ReturnStmtT;
    pAST exp = nullptr;

    // type of the return exp
    string info() const override {
        if (exp == nullptr) return "void";
        return exp->info();
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ReturnStmtAST";
        if (exp != nullptr) {
            j["exp"] = exp->toJson();
        }
        return j;
    }
};

class ParenExpAST : public ExpAST {
   public:
    TType ty = TType::ParenExpT;
    pAST p;

    ParenExpAST(BaseAST *ast) { p = pAST(ast); }

    int eval() const override {
        auto exp = reinterpret_cast<ExpAST *>(p.get());
        return exp->eval();
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ParenExpAST";
        if (p != nullptr) {
            j["p"] = p->toJson();
        }
        return j;
    }
};

class NumberAST : public ExpAST {
   public:
    TType ty = TType::NumberT;
    int num;

    NumberAST(int n) { num = n; }

    int eval() const override { return num; }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "NumberAST";
        j["num"] = num;
        return j;
    }
};

class BTypeAST : public BaseAST {
   public:
    TType ty = TType::BTypeT;
    // can be void if it is a function return type
    string elementType = "int";
    // -1 for "[]" (pointer), can be nullptr or empty
    unique_ptr<vector<int>> dims = nullptr;

    // get type of llvm format
    // like [10 x [10 x i32*]] for [10][10][]int
    string info() const override {
        string ret;
        if (elementType == "int") {
            ret = "i32";
        } else if (elementType == "void") {
            return "void";
        } else {
            cerr << "BTypeAST error: unknown element type" << endl;
            assert(false);
        }
        if (dims != nullptr && !dims->empty()) {
            for (auto &dim : *dims) {
                if (dim == -1) {
                    ret += "*";
                } else {
                    ret = "[" + to_string(dim) + " x " + ret + "]";
                }
            }
        }
        return ret;
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BTypeAST";
        j["elementType"] = elementType;
        if (dims != nullptr && !dims->empty()) {
            j["dims"] = json::array();
            for (auto &dim : *dims) {
                j["dims"].push_back(dim);
            }
        }
        return j;
    }
};

class UnaryExpAST : public ExpAST {
   public:
    TType ty = TType::UnaryExpT;
    // unary operator, '+' '-' '!'
    char op = ' ';
    pAST p = nullptr;

    UnaryExpAST(char _op, BaseAST *ast) {
        op = _op;
        p = pAST(ast);
    }

    int eval() const override {
        auto exp = reinterpret_cast<ExpAST *>(p.get());
        int val = exp->eval();
        switch (op) {
            case '+':
                return val;
            case '-':
                return -val;
            case '!':
                return !val;
            default:
                cerr << "error: unknown unary operator" << endl;
                assert(false);
        }
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "UnaryExpAST";
        j["op"] = string(1, op);
        if (p != nullptr) {
            j["p"] = p->toJson();
        }
        return j;
    }
};

class CallExpAST : public ExpAST {
   public:
    TType ty = TType::CallExpT;
    string funcName = "";
    pvpAST argList = nullptr;

    CallExpAST(string *_funcName, vpAST *_argList) {
        funcName = *unique_ptr<string>(_funcName);
        argList = pvpAST(_argList);
    }

    int eval() const override {
        cerr << "error: call expression is not const" << endl;
        assert(false);
        return -1;
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "CallExpAST";
        if (funcName != "") {
            j["funcName"] = funcName;
        }
        if (argList != nullptr) {
            j["argList"] = json::array();
            for (auto &arg : *argList) {
                j["argList"].push_back(arg->toJson());
            }
        }
        return j;
    }
};

class ForStmtAST : public StmtAST {
   public:
    TType ty = TType::ForStmtT;
    pAST init = make_unique<EmptyStmtAST>();
    pAST cond = make_unique<NumberAST>(1);
    pAST post = make_unique<EmptyStmtAST>();
    pAST body;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ForStmtAST";
        j["init"] = init->toJson();
        j["cond"] = cond->toJson();
        j["post"] = post->toJson();
        j["body"] = body->toJson();
        return j;
    }
};

// now include Assign, TODO rename
class ShortVarDeclAST : public StmtAST {
   public:
    TType ty = TType::ShortVarDeclT;
    bool isDefine = false;
    pvpAST targets;
    pvpAST initVals;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ShortVarDecl";
        j["isDefine"] = isDefine;
        j["Targets"] = json::array();
        for (auto &tar : *targets) {
            j["targets"].push_back(tar->toJson());
        }
        j["initVals"] = json::array();
        for (auto &initVal : *initVals) {
            j["initVals"].push_back(initVal->toJson());
        }
        return j;
    }
};

class LValAST : public ExpAST {
   public:
    TType ty = TType::LValT;
    string ident;
    pvpAST indexList; // cannot be nullptr
    // only generated by := stmt
    string typeInfo = "";
    // TODO support const
    // bool isConst = false;

    void add(BaseAST *index) override {
        if (indexList == nullptr) {
            // actually impossible
            indexList = make_unique<vpAST>();
        }
        indexList->push_back(pAST(index));
    }

    int eval() const override {
        // TODO support const var
        cerr << "error: lval is not const" << endl;
        assert(false);
    }
    string info() const override {
        if (typeInfo == "") {
            cerr << "error: not var defined with :=" << endl;
            cerr << "please use inferType instead" << endl;
            assert(false);
        }
        return typeInfo;
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "LValAST";
        j["ident"] = ident;
        if (indexList->size() > 0) {
            j["indexList"] = json::array();
            for (auto &index : *indexList) {
                j["indexList"].push_back(index->toJson());
            }
        }
        return j;
    }
};

class BinExpAST : public ExpAST {
   public:
    TType ty = TType::BinExpT;
    enum Op {
        ADD,  // +
        SUB,  // -
        MUL,  // *
        DIV,  // /
        MOD,  // %
        EQ,   // ==
        NE,   // !=
        LT,   // < // str
        LE,   // <=
        GT,   // > // str
        GE,   // >=
        AND,  // &&
        OR,   // ||
    } op;
    string opStr;
    pAST left;
    pAST right;

    BinExpAST(char _op, BaseAST *ast1, BaseAST *ast2) {
        // assign op according to + - * / ...
        opStr = string(1, _op);
        if (_op == '+') {
            op = Op::ADD;
        } else if (_op == '-') {
            op = Op::SUB;
        } else if (_op == '*') {
            op = Op::MUL;
        } else if (_op == '/') {
            op = Op::DIV;
        } else if (_op == '%') {
            op = Op::MOD;
        } else {
            cerr << "BinExpAST: unknown char op" << endl;
            assert(false);
        }
        left = pAST(ast1);
        right = pAST(ast2);
    }
    BinExpAST(string *_op, BaseAST *ast1, BaseAST *ast2) {
        // assign op according to == != < <= > >= && ||
        opStr = *_op;
        if (*_op == "==") {
            op = Op::EQ;
        } else if (*_op == "!=") {
            op = Op::NE;
        } else if (*_op == "<") {
            op = Op::LT;
        } else if (*_op == "<=") {
            op = Op::LE;
        } else if (*_op == ">") {
            op = Op::GT;
        } else if (*_op == ">=") {
            op = Op::GE;
        } else if (*_op == "&&") {
            op = Op::AND;
        } else if (*_op == "||") {
            op = Op::OR;
        } else {
            cerr << "BinExpAST: unknown string op" << endl;
            assert(false);
        }
        left = pAST(ast1);
        right = pAST(ast2);
    }

    int eval() const override {
        auto lExp = dynamic_cast<const ExpAST *>(left.get());
        auto rExp = dynamic_cast<const ExpAST *>(right.get());
        if (op == Op::ADD) {
            return lExp->eval() + rExp->eval();
        } else if (op == Op::SUB) {
            return lExp->eval() - rExp->eval();
        } else if (op == Op::MUL) {
            return lExp->eval() * rExp->eval();
        } else if (op == Op::DIV) {
            return lExp->eval() / rExp->eval();
        } else if (op == Op::MOD) {
            return lExp->eval() % rExp->eval();
        } else if (op == Op::EQ) {
            return lExp->eval() == rExp->eval();
        } else if (op == Op::NE) {
            return lExp->eval() != rExp->eval();
        } else if (op == Op::LT) {
            return lExp->eval() < rExp->eval();
        } else if (op == Op::LE) {
            return lExp->eval() <= rExp->eval();
        } else if (op == Op::GT) {
            return lExp->eval() > rExp->eval();
        } else if (op == Op::GE) {
            return lExp->eval() >= rExp->eval();
        } else if (op == Op::AND) {
            return lExp->eval() && rExp->eval();
        } else if (op == Op::OR) {
            return lExp->eval() || rExp->eval();
        } else {
            cerr << "error: unknown op" << endl;
            assert(false);
            return -1;
        }
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BinaryExpAST";
        j["op"] = opStr;
        j["left"] = left->toJson();
        j["right"] = right->toJson();
        return j;
    }
};

class IfStmtAST : public StmtAST {
   public:
    TType ty = TType::IfStmtT;
    enum Type { If, IfElse, IfElseIf } t;
    pAST init = nullptr;
    pAST cond;
    pAST body;
    // else block or another if stmt
    pAST elseBlockStmt = nullptr;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "IfStmtAST";
        j["t"] = t;
        if (init != nullptr) {
            j["init"] = init->toJson();
        }
        j["cond"] = cond->toJson();
        j["body"] = body->toJson();
        if (elseBlockStmt != nullptr) {
            j["elseBlockStmt"] = elseBlockStmt->toJson();
        }
        return j;
    }
};

class ExpStmtAST : public StmtAST {
   public:
    TType ty = TType::ExpStmtT;
    pAST exp;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ExpStmtAST";
        j["exp"] = exp->toJson();
        return j;
    }
};

class BranchStmtAST : public StmtAST {
   public:
    TType ty = TType::BranchStmtT;
    enum Type { Break, Continue, Goto } t;
    string ident = "";

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BranchStmtAST";
        j["t"] = t;
        if (t == Type::Goto) {
            j["ident"] = ident;
        }
        return j;
    }
};

class IncDecStmtAST : public StmtAST {
   public:
    TType ty = TType::IncDecStmtT;
    pAST target;
    bool isInc;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "IncDecStmtAST";
        j["target"] = target->toJson();
        j["isInc"] = isInc;
        return j;
    }
};