#pragma once

#include <bits/stdc++.h>

#include <iostream>
#include <memory>
#include <string>

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
};

class PackClauseAST : public BaseAST {
   public:
    string ident;

    ostream &Dump(ostream &os) const override {
        os << "PackClauseAST { " << ident << " }";
        return os;
    }
};

class FuncTypeAST : public BaseAST {
   public:
    string type = "void";  // default void

    ostream &Dump(ostream &os) const override {
        os << "FuncTypeAST { " << type << " }";
        return os;
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
};

class StmtListAST : public BaseAST {
   public:
    pvpAST stmts;
};

class StmtAST : public BaseAST {};

class ReturnStmtAST : public StmtAST {
   public:
    pAST exp;

    ostream &Dump(ostream &os) const override {
        os << "ReturnStmtAST { " << *exp << " }";
        return os;
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
};