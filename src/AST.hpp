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
    virtual ostream &Dump(ostream &os) const = 0;
    static ostream &DumpList(ostream &os, const vector<unique_ptr<BaseAST>> &list) {
        for (int i = 0; i < list.size(); i++) {
            os << *list[i];
            if (i != list.size() - 1) {
                os << ", ";
            }
        }
        return os;
    }
    virtual json toJson() const = 0;
    friend ostream &operator<<(ostream &os, const BaseAST &ast) {
        return ast.Dump(os);
    }
};

using pAST = unique_ptr<BaseAST>;
using vpAST = vector<pAST>;
using pvpAST = unique_ptr<vpAST>;

// dump list with no ending comma

class CompUnitAST : public BaseAST {
   public:
    // 用智能指针管理对象
    pvpAST topDefs;
    ostream &Dump(ostream &os) const override {
        os << "CompUnitAST { ";
        DumpList(os, *topDefs);
        os << " }";
        return os;
    }
    json toJson() const override {
        json j;
        j["type"] = "CompUnitAST";
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

    ostream &Dump(ostream &os) const override {
        os << "FuncDefAST { " << ident << ", " << *func_type << ", " << *block
           << " }";
        return os;
    }
    json toJson() const override {
        json j;
        j["type"] = "FuncDefAST";
        j["func_type"] = func_type->toJson();
        j["ident"] = ident;
        j["block"] = block->toJson();
        return j;
    }
};

class PackClauseAST : public BaseAST {
   public:
    string ident;

    ostream &Dump(ostream &os) const override {
        os << "PackClauseAST { " << ident << " }";
        return os;
    }

    json toJson() const override {
        json j;
        j["type"] = "PackClauseAST";
        j["ident"] = ident;
        return j;
    }
};

class FuncTypeAST : public BaseAST {
   public:
    string type = "void";  // default void

    ostream &Dump(ostream &os) const override {
        os << "FuncTypeAST { " << type << " }";
        return os;
    }

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

    ostream &Dump(ostream &os) const override {
        os << "BlockAST { ";
        DumpList(os, *stmts);
        os << " }";
        return os;
    }

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

class ReturnStmtAST : public StmtAST {
   public:
    pAST exp;

    ostream &Dump(ostream &os) const override {
        os << "ReturnStmtAST { " << *exp << " }";
        return os;
    }

    json toJson() const override {
        json j;
        j["type"] = "ReturnStmtAST";
        j["exp"] = exp->toJson();
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
    ostream &Dump(ostream &os) const override {
        os << "PrimaryExpAST { ";
        if (t == PAREN) {
            os << *p;
        } else if (t == LVAL) {
            os << *p;
        } else if (t == NUM) {
            os << num;
        }
        os << " }";
        return os;
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