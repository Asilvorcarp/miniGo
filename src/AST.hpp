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
    FuncTypeT,
    BlockT,
    // StmtT,
    VarSpecT,
    EmptyStmtT,
    ExpStmtT,
    ReturnStmtT,
    BTypeT,
    ForStmtT,
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
    _0,
    _1
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
};

using pAST = unique_ptr<BaseAST>;
using vpAST = vector<pAST>;
using pvpAST = unique_ptr<vpAST>;
using pvStr = unique_ptr<vector<string>>;

class StmtAST : public BaseAST {};
class ExpAST : public BaseAST {};

class FuncDefAST : public BaseAST {
   public:
    TType ty = TType::FuncDefT;
    pAST func_type;
    string ident;
    pAST body;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "FuncDefAST";
        j["func_type"] = func_type->toJson();
        j["ident"] = ident;
        j["body"] = body->toJson();
        return j;
    }
};

class VarSpecAST : public BaseAST {
   public:
    TType ty = TType::VarSpecT;
    pvStr idents;
    pAST btype = nullptr;
    pvpAST initVals = make_unique<vpAST>();

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

class CompUnitAST : public BaseAST {
   public:
    TType ty = TType::CompUnitT;
    string packageName;
    vector<unique_ptr<VarSpecAST>> Globals;
    vector<unique_ptr<FuncDefAST>> Funcs;

    void setDefs(vpAST *topDeclList) {
        cout << ">> FUCK" << endl;
        for (auto &topDef : *topDeclList) {
            if (topDef->type() == TType::VarSpecT) {
                cout << ">> FUCK1" << endl;
                Globals.push_back(
                    unique_ptr<VarSpecAST>((VarSpecAST *)topDef.get()));
            } else if (topDef->type() == TType::FuncDefT) {
                cout << ">> FUCK2" << endl;
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

class FuncTypeAST : public BaseAST {
   public:
    TType ty = TType::FuncTypeT;
    string t = "void";  // default void

    TType type() const override { return ty; }

    json toJson() const override {
        json j;
        j["type"] = "FuncTypeAST";
        j["t"] = t;
        return j;
    }
};

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
    string elementType = "int";
    unique_ptr<vector<int>> dims = nullptr;

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BType";
        j["elementType"] = elementType;
        if (dims != nullptr) {
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

// class AssignStmtAST : public StmtAST {
//    public:
//     TType ty = TType::AssignStmtT;
//     pAST lval;
//     pAST exp;

//     TType type() const override { return ty; }
//     json toJson() const override {
//         json j;
//         j["type"] = "AssignStmtAST";
//         j["lval"] = lval->toJson();
//         j["exp"] = exp->toJson();
//         return j;
//     }
// };

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
    pvpAST indexList;

    void add(BaseAST *index) override {
        if (indexList == nullptr) {
            // actually impossible
            indexList = make_unique<vpAST>();
        }
        indexList->push_back(pAST(index));
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
    pAST left;
    pAST right;

    BinExpAST(char _op, BaseAST *ast1, BaseAST *ast2) {
        // assign op according to + - * / ...
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

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BinaryExpAST";
        j["op"] = op;
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