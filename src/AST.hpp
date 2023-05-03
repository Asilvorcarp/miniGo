#pragma once

#include <bits/stdc++.h>

#include <iostream>
#include <memory>
#include <string>

#include <json.hpp>

using json = nlohmann::json;
using namespace std;

// 所有 AST 的基类
class BaseAST {
   public:
    virtual ~BaseAST() = default;
    virtual json toJson() const = 0;
    friend ostream &operator<<(ostream &os, const BaseAST &ast) {
        return os << ast.toJson().dump(4);
    }
    virtual void add(BaseAST *ast) {};
};

using pAST = unique_ptr<BaseAST>;
using vpAST = vector<pAST>;
using pvpAST = unique_ptr<vpAST>;
using pvStr = unique_ptr<vector<string>>;

// dump list with no ending comma

class CompUnitAST : public BaseAST {
   public:
    string packageName;
    pvpAST topDefs;

    json toJson() const override {
        json j;
        j["type"] = "CompUnitAST";
        j["packageName"] = packageName;
        j["topDefs"] = json::array();
        for (auto &topDef : *topDefs) {
            j["topDefs"].push_back(topDef->toJson());
        }
        return j;
    }
};

class FuncDefAST : public BaseAST {
   public:
    pAST func_type;
    string ident;
    pAST block;

    json toJson() const override {
        json j;
        j["type"] = "FuncDefAST";
        j["func_type"] = func_type->toJson();
        j["ident"] = ident;
        j["block"] = block->toJson();
        return j;
    }
};

class FuncTypeAST : public BaseAST {
   public:
    string type = "void";  // default void

    json toJson() const override {
        json j;
        j["type"] = "FuncTypeAST";
        j["type"] = type;
        return j;
    }
};

class BlockAST : public BaseAST {
   public:
    pvpAST stmts;

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

class StmtAST : public BaseAST {};

class EmptyStmtAST : public StmtAST {
   public:
    json toJson() const override {
        json j;
        j["type"] = "EmptyStmtAST";
        return j;
    }
};

class ReturnStmtAST : public StmtAST {
   public:
    pAST exp = nullptr;

    json toJson() const override {
        json j;
        j["type"] = "ReturnStmtAST";
        if (exp != nullptr) {
            j["exp"] = exp->toJson();
        }
        return j;
    }
};

class PrimaryExpAST : public BaseAST {
   public:
    enum Type { PAREN, LVAL, NUM };
    Type t;
    pAST p;
    int num;

    PrimaryExpAST(int _num) {
        t = NUM;
        num = _num;
    }
    PrimaryExpAST(BaseAST *ast, Type _t) {
        t = _t;
        p = pAST(ast);
    }

    json toJson() const override {
        json j;
        j["type"] = "PrimaryExpAST";
        if (t == PAREN) {
            j["p"] = p->toJson();
        } else if (t == LVAL) {
            j["p"] = p->toJson();
        } else if (t == NUM) {
            j["num"] = num;
        }
        return j;
    }
};

class BTypeAST : public BaseAST {
    public:
    string elementType = "int";
    unique_ptr<vector<int>> dims = nullptr;

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

class VarSpecAST : public BaseAST {
   public:
    pvStr idents;
    pAST btype = nullptr;
    pvpAST initVals = nullptr;

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

class UnaryExpAST : public BaseAST {
    public:
    // unary operator, '+' '-' '!' or 'C' for func call
    char op;
    pAST p = nullptr;
    string funcName = "";
    pvpAST argList = nullptr;

    UnaryExpAST(char _op, BaseAST *ast) {
        op = _op;
        p = pAST(ast);
    }
    UnaryExpAST(string* _funcName, vpAST *_argList) {
        op = 'C';
        funcName = *unique_ptr<string>(_funcName);
        argList = pvpAST(_argList);
    }

    json toJson() const override {
        json j;
        j["type"] = "UnaryExpAST";
        j["op"] = op;
        if (p != nullptr) {
            j["p"] = p->toJson();
        }
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
    pAST init = make_unique<EmptyStmtAST>();
    pAST cond = make_unique<PrimaryExpAST>(1);
    pAST step = make_unique<EmptyStmtAST>();
    pAST block;

    json toJson() const override {
        json j;
        j["type"] = "ForStmtAST";
        j["init"] = init->toJson();
        j["cond"] = cond->toJson();
        j["step"] = step->toJson();
        j["block"] = block->toJson();
        return j;
    }
};

class AssignStmtAST : public StmtAST {
   public:
    pAST lval;
    pAST exp;

    json toJson() const override {
        json j;
        j["type"] = "AssignStmtAST";
        j["lval"] = lval->toJson();
        j["exp"] = exp->toJson();
        return j;
    }
};

class ShortVarDeclAST : public StmtAST {
   public:
    pvStr idents;
    pvpAST initVals;

    json toJson() const override {
        json j;
        j["type"] = "ShortVarDecl";
        j["idents"] = json::array();
        for (auto &ident : *idents) {
            j["idents"].push_back(ident);
        }
        j["initVals"] = json::array();
        for (auto &initVal : *initVals) {
            j["initVals"].push_back(initVal->toJson());
        }
        return j;
    }
};

class LValAST : public BaseAST {
   public:
    string ident;
    pvpAST indexList;

    void add(BaseAST* index) override {
        if (indexList == nullptr) { 
            // actually impossible
            indexList = make_unique<vpAST>();
        }
        indexList->push_back(pAST(index));
    }

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

class BinExpAST : public BaseAST {
   public:
    string op;
    pAST left;
    pAST right;

    BinExpAST(char _op, BaseAST *ast1, BaseAST *ast2) {
        op = _op; // TODO test
        left = pAST(ast1);
        right = pAST(ast2);
    }
    BinExpAST(string* _op, BaseAST *ast1, BaseAST *ast2) {
        op = *unique_ptr<string>(_op);
        left = pAST(ast1);
        right = pAST(ast2);
    }

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
    enum Type { If, IfElse, IfElseIf } t;
    pAST beforeStmt = nullptr;
    pAST cond;
    pAST thenBlock;
    // else block or if stmt
    pAST elseBlockStmt = nullptr;

    json toJson() const override {
        json j;
        j["type"] = "IfStmtAST";
        j["t"] = t;
        if (beforeStmt != nullptr) {
            j["beforeStmt"] = beforeStmt->toJson();
        }
        j["cond"] = cond->toJson();
        j["thenBlock"] = thenBlock->toJson();
        if (elseBlockStmt != nullptr) {
            j["elseBlockStmt"] = elseBlockStmt->toJson();
        }
        return j;
    }
};