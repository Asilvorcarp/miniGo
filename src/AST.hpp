#pragma once

#include <iostream>
#include <memory>
#include <string>

using namespace std;

// 所有 AST 的基类
class BaseAST {
   public:
    virtual ~BaseAST() = default;
    virtual ostream &Dump(ostream &os) const = 0;
    friend ostream &operator<<(ostream &os, const BaseAST &ast) {
        return ast.Dump(os);
    }
};

class CompUnitAST : public BaseAST {
   public:
    // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;
    ostream &Dump(ostream &os) const override {
        os << "CompUnitAST { ";
        os << *func_def;
        os << " }";
        return os;
    }
};

class FuncDefAST : public BaseAST {
   public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    ostream &Dump(ostream &os) const override {
        os << "FuncDefAST { " << ident << ", " << *func_type << ", " << *block
           << " }";
        return os;
    }
};

class PackDefAST : public BaseAST {
   public:
    string ident;

    ostream &Dump(ostream &os) const override {
        os << "PackDefAST { " << ident << " }";
        return os;
    }
};

class FuncTypeAST : public BaseAST {
   public:
    string type = "int";

    ostream &Dump(ostream &os) const override {
        os << "FuncTypeAST { " << type << " }";
        return os;
    }
};

class BlockAST : public BaseAST {
   public:
    unique_ptr<BaseAST> stmt;

    ostream &Dump(ostream &os) const override {
        os << "BlockAST { " << *stmt << " }";
        return os;
    }
};

class StmtAST : public BaseAST {
   public:
    // now only support return
    int number = 0;

    ostream &Dump(ostream &os) const override {
        os << "StmtAST { return " << number << " }";
        return os;
    }
};
