#pragma once

#include <iostream>
#include <json.hpp>
#include <memory>
#include <string>

using json = nlohmann::json;
using namespace std;

extern int yydebug;

/**
 * @brief the types of the AST
 */
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
    RuntimeFuncT,
    MakeExpT,
    ArrayExpT,
    NilT,
    // for future use
    _0,
    _1,
    _2,
    _3,
    _4,
};

/**
 * @brief the base class of all ASTs
 */
class BaseAST {
   protected:
    /**
     * @brief counter for some ASTs to generate unique id
     * @note currently used in ForStmtAST
     */
    static uint counter;

   public:
    /**
     * @brief the parent of the AST
     * @note need to be set when constructing the AST
     */
    BaseAST *parent = nullptr;
    /**
     * @brief Destroy the BaseAST object
     */
    virtual ~BaseAST() = default;
    /**
     * @brief get the type of the AST
     * @return TType - the type of the AST
     */
    virtual TType type() const = 0;
    /**
     * @brief get the json representation of the AST
     * 
     * @return json 
     */
    virtual json toJson() const = 0;
    /**
     * @brief get the string representation of the AST in json format
     */
    friend ostream &operator<<(ostream &os, const BaseAST &ast) {
        return os << ast.toJson().dump(4);
    }
    /**
     * @brief Set the Parent object 
     * 
     * @param parent 
     */
    void setParent(BaseAST *parent) { this->parent = parent; }
    /**
     * @brief Get the Parent object
     * 
     * @return BaseAST* 
     */
    BaseAST *getParent() { return parent; }
    /**
     * @brief add a child to the AST
     * @note currently used in LValAST to add index
     * @note not all ASTs implement this
     * 
     * @param ast 
     */
    virtual void add(BaseAST *ast){};
    /**
     * @brief get the type info of the AST
     * @note not all ASTs implement this
     * 
     * @return string - the type info
     */
    virtual string info() const { return "base"; }
    /**
     * @brief copy the AST itself
     * @note not all ASTs implement this
     * 
     * @return BaseAST* - the copied AST
     */
    virtual BaseAST *copy() const { return nullptr; }
};

using pT = BaseAST *;
using pAST = unique_ptr<BaseAST>;
using vpAST = vector<pAST>;
using pvpT = vector<pAST> *;
using pvpAST = unique_ptr<vpAST>;
using pvStr = unique_ptr<vector<string>>;
using pStr = unique_ptr<string>;

/**
 * @brief the base AST of all the statements
 */
class StmtAST : public BaseAST {};

/**
 * @brief the base AST of all the expressions
 */
class ExpAST : public BaseAST {
   public:
    virtual int eval() const = 0;
};

/**
 * @brief AST of variable specification
 * @note Example: "var a, b, c int = 1, 2, 3"
 * @note the type of the variable may be inferred from the init value
 */
class VarSpecAST : public BaseAST {
   public:
    TType ty = TType::VarSpecT;
    pvStr idents;
    pAST btype = nullptr;  // maybe nullptr
    pvpAST initVals = make_unique<vpAST>();

    VarSpecAST(vector<string> *idents, pT btype, pvpT initVals)
        : idents(pvStr(idents)) {
        if (btype == nullptr) {
            this->btype = nullptr;
        } else {
            this->btype = pAST(btype);
            btype->setParent(this);
        }
        if (initVals == nullptr) {
            this->initVals = make_unique<vpAST>();
        } else {
            for (auto &initVal : *this->initVals) {
                initVal->setParent(this);
            }
            this->initVals = pvpAST(initVals);
        }
    }

    // return "infer" if btype is nullptr (not specified in code)
    virtual string info() const override {
        if (btype == nullptr) return "infer";
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

/**
 * @brief AST of function definition
 * @note Example: "func f(a int, b int) int { return a + b }"
 */
class FuncDefAST : public BaseAST {
   public:
    TType ty = TType::FuncDefT;
    string ident;
    pvpAST paramList;
    pAST retType;  // BType
    pAST body;

    FuncDefAST() = default;  // only for runtime func
    FuncDefAST(string *ident, pvpT paramList, pT retType, pT body)
        : ident(*pStr(ident)),
          paramList(pvpAST(paramList)),
          retType(pAST(retType)),
          body(pAST(body)) {
        for (auto &param : *this->paramList) {
            param->setParent(this);
        }
        this->retType->setParent(this);
        this->body->setParent(this);
    }

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

/**
 * @brief AST of a runtime function
 * @note mainly for getting the types of return value and parameters easily
 * from a AST node of a object in a scope
 */
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

/**
 * @brief the AST of a compilation unit, usually a file
 * @note maybe a package in the future
 */
class CompUnitAST : public BaseAST {
   public:
    TType ty = TType::CompUnitT;
    string packageName;
    vector<unique_ptr<VarSpecAST>> Globals;
    vector<unique_ptr<FuncDefAST>> Funcs;

    void setDefs(vpAST *topDeclList) {
        for (auto &topDef : *topDeclList) {
            topDef.get()->setParent(this);
            if (topDef->type() == TType::VarSpecT) {
                Globals.push_back(
                    unique_ptr<VarSpecAST>((VarSpecAST *)topDef.get()));
            } else if (topDef->type() == TType::FuncDefT) {
                Funcs.push_back(
                    unique_ptr<FuncDefAST>((FuncDefAST *)topDef.get()));
            } else {
                cerr << "unknown topDef type" << endl;
                assert(false);
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

/**
 * @brief AST of a parameter in a function
 */
class ParamAST : public BaseAST {
   public:
    TType ty = TType::ParamT;
    string ident;
    pAST t;  // BType

    ParamAST(string *ident, BaseAST *t) {
        this->ident = *pStr(ident);
        this->t = pAST(t);
        t->setParent(this);
    }

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

/**
 * @brief AST of a block
 * @note a block is a list of statements
 */
class BlockAST : public BaseAST {
   public:
    TType ty = TType::BlockT;
    pvpAST stmts;

    BlockAST(pvpT stmts) : stmts(pvpAST(stmts)) {
        for (auto &stmt : *stmts) {
            stmt->setParent(this);
        }
    }

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

/**
 * @brief AST of a empty statement 
 * @note Example: ";"
 */
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

/**
 * @brief AST of return statement
 * @note Example: "return 1;"
 */
class ReturnStmtAST : public StmtAST {
   public:
    TType ty = TType::ReturnStmtT;
    pAST exp = nullptr;

    ReturnStmtAST() = delete;
    ReturnStmtAST(pT exp) {
        if (exp != nullptr) {
            this->exp = pAST(exp);
            exp->setParent(this);
        }
    }

    // not valid, just tell if need to infer
    string info() const override {
        if (exp == nullptr) return "void";
        return "infer";
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

/**
 * @brief AST of a parenthesized expression
 * @note Example: "(1 + 2)"
 */
class ParenExpAST : public ExpAST {
   public:
    TType ty = TType::ParenExpT;
    pAST p;

    ParenExpAST(BaseAST *ast) {
        p = pAST(ast);
        ast->setParent(this);
    }

    int eval() const override {
        auto exp = reinterpret_cast<ExpAST *>(p.get());
        return exp->eval();
    }
    BaseAST *copy() const override { return new ParenExpAST(p->copy()); }
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

/**
 * @brief AST of a number
 */
class NumberAST : public ExpAST {
   public:
    TType ty = TType::NumberT;
    int num;

    NumberAST(int n) { num = n; }

    int eval() const override { return num; }
    BaseAST *copy() const override { return new NumberAST(num); }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "NumberAST";
        j["num"] = num;
        return j;
    }
};

/**
 * @brief AST of a (base) type
 */
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
            ret = "i64";
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
    BaseAST *copy() const override {
        auto ret = new BTypeAST();
        ret->elementType = elementType;
        if (dims != nullptr) {
            auto newDims = new vector<int>(*dims);
            ret->dims = unique_ptr<vector<int>>(newDims);
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

/**
 * @brief AST of a unary expression
 * @note Example: "-x" "!(x + 1)"
 */
class UnaryExpAST : public ExpAST {
   public:
    TType ty = TType::UnaryExpT;
    // unary operator, '+' '-' '!'
    char op = ' ';
    pAST p = nullptr;

    UnaryExpAST() = delete;
    UnaryExpAST(char _op, BaseAST *ast) {
        ast->setParent(this);
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
                return -1;
        }
    }
    BaseAST *copy() const override { return new UnaryExpAST(op, p->copy()); }
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

/**
 * @brief AST of a call expression
 * @note Example: "f(1, 2, 3)"
 */
class CallExpAST : public ExpAST {
   public:
    TType ty = TType::CallExpT;
    string funcName = "";
    pvpAST argList = nullptr;

    CallExpAST(string *_funcName, vpAST *_argList) {
        funcName = *unique_ptr<string>(_funcName);
        argList = pvpAST(_argList);
        // set parent
        for (auto &arg : *argList) {
            arg->setParent(this);
        }
    }

    int eval() const override {
        cerr << "error: call expression is not const" << endl;
        assert(false);
        return -1;
    }
    BaseAST *copy() const override {
        auto newArgList = new vector<pAST>();
        for (auto &arg : *argList) {
            newArgList->push_back(pAST(arg->copy()));
        }
        return new CallExpAST(new string(funcName), newArgList);
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

/**
 * @brief AST of a for statement
 * @note This includes always/while in golang
 */
class ForStmtAST : public StmtAST {
   public:
    TType ty = TType::ForStmtT;
    // id for labels
    uint id;
    pAST init = make_unique<EmptyStmtAST>();
    pAST cond = make_unique<NumberAST>(1);
    pAST post = make_unique<EmptyStmtAST>();
    pAST body;  // cannot be nullptr

    ForStmtAST() = delete;
    ForStmtAST(pT _init, pT _cond, pT _post, pT _body) {
        id = BaseAST::counter++;
        if (_init == nullptr)
            init = make_unique<EmptyStmtAST>();
        else
            init = pAST(_init);
        if (_cond == nullptr)
            cond = make_unique<NumberAST>(1);
        else
            cond = pAST(_cond);
        if (_post == nullptr)
            post = make_unique<EmptyStmtAST>();
        else
            post = pAST(_post);
        body = pAST(_body);
        // set parent
        init.get()->setParent(this);
        cond.get()->setParent(this);
        post.get()->setParent(this);
        body.get()->setParent(this);
    }

    // get label of different suffix,
    // including cond, post, body, end
    string getLabel(string suffix) {
        auto labels = new vector<string>();
        stringstream ss;
        ss << "for." << id << "." << suffix;
        return ss.str();
    }

    TType type() const override { return ty; }
    json toJson() const override {
        if (yydebug) cout << "ForStmtAST::toJson" << endl;
        json j;
        j["type"] = "ForStmtAST";
        if (init != nullptr) {
            j["init"] = init->toJson();
        } else {
            cerr << "error: init is nullptr" << endl;
            assert(false);
        }
        if (cond != nullptr) {
            j["cond"] = cond->toJson();
        } else {
            cerr << "error: cond is nullptr" << endl;
            assert(false);
        }
        if (post != nullptr) {
            j["post"] = post->toJson();
        } else {
            cerr << "error: post is nullptr" << endl;
            assert(false);
        }
        if (body != nullptr) {
            j["body"] = body->toJson();
        } else {
            cerr << "error: body is nullptr" << endl;
            assert(false);
        }
        return j;
    }
};

/**
 * @brief AST of a binary expression
 * @note Example: "1 + 2"
 */
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
        ast1->setParent(this);
        ast2->setParent(this);
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
        } else if (*_op == "+") {
            op = Op::ADD;
        } else if (*_op == "-") {
            op = Op::SUB;
        } else if (*_op == "*") {
            op = Op::MUL;
        } else if (*_op == "/") {
            op = Op::DIV;
        } else if (*_op == "%") {
            op = Op::MOD;
        } else {
            cerr << "BinExpAST: unknown string op" << endl;
            assert(false);
        }
        ast1->setParent(this);
        ast2->setParent(this);
        left = pAST(ast1);
        right = pAST(ast2);
    }

    int eval() const override {
        auto lExp = reinterpret_cast<const ExpAST *>(left.get());
        auto rExp = reinterpret_cast<const ExpAST *>(right.get());
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
    BaseAST *copy() const override {
        return new BinExpAST(new string(opStr), left->copy(), right->copy());
    }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "BinaryExpAST";
        j["op"] = opStr;
        if (left != nullptr) j["left"] = left->toJson();
        j["right"] = right->toJson();
        return j;
    }
};

/**
 * @brief AST of assignment, including = and :=
 */
class ShortVarDeclAST : public StmtAST {
   public:
    TType ty = TType::ShortVarDeclT;
    bool isDefine = false;
    pvpAST targets = nullptr;
    pvpAST initVals = nullptr;

    ShortVarDeclAST() = delete;
    ShortVarDeclAST(bool _isDefine, pvpT _targets, pvpT _initVals)
        : isDefine(_isDefine), targets(pvpAST(_targets)), initVals(_initVals) {
        for (auto &tar : *targets) {
            tar->setParent(this);
        }
        for (auto &initVal : *initVals) {
            initVal->setParent(this);
        }
    }
    // +=, -=, *=, /=, %=: only one target and one initVal
    ShortVarDeclAST(BaseAST *target, char _op, BaseAST *initVal) {
        isDefine = false;
        target->setParent(this);
        targets = make_unique<vector<pAST>>();
        targets->push_back(pAST(target));
        auto target2 = target->copy();
        auto initVal2 = initVal->copy();
        auto binInit = new BinExpAST(_op, target2, initVal2);
        binInit->setParent(this);
        initVals = make_unique<vector<pAST>>();
        initVals->push_back(pAST(binInit));
    }

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ShortVarDecl";
        j["isDefine"] = isDefine;
        if (targets != nullptr) {
            j["targets"] = json::array();
            for (auto &tar : *targets) {
                j["targets"].push_back(tar->toJson());
            }
        }
        if (initVals != nullptr) {
            j["initVals"] = json::array();
            for (auto &initVal : *initVals) {
                j["initVals"].push_back(initVal->toJson());
            }
        }
        return j;
    }
};

/**
 * @brief AST of a left value
 * @note Example: a[1][2]
 */
class LValAST : public ExpAST {
   public:
    TType ty = TType::LValT;
    string ident;
    pvpAST indexList = nullptr;  // cannot be nullptr
    // only generated by := stmt or var spec
    string typeInfo = "";
    // TODO support const
    // bool isConst = false;

    LValAST(string *_ident, pvpT _indexList) : ident(*pStr(_ident)) {
        if (_indexList == nullptr) {
            indexList = make_unique<vpAST>();
        } else {
            for (auto &index : *indexList) {
                index->setParent(this);
            }
            indexList = pvpAST(_indexList);
        }
    }
    LValAST(string ident, pvpT list, string typeInfo)
        : ident(ident), typeInfo(typeInfo) {
        indexList = pvpAST(list);
        for (auto &index : *indexList) {
            index->setParent(this);
        }
    }
    // store typeInfo as node in obj
    LValAST(string ident, string typeInfo) : ident(ident), typeInfo(typeInfo) {
        indexList = make_unique<vpAST>();
    }

    void add(BaseAST *index) override {
        index->setParent(this);
        if (indexList == nullptr) {
            // actually impossible
            indexList = make_unique<vpAST>();
        }
        indexList->push_back(pAST(index));
    }
    BaseAST *copy() const override {
        auto list = new vpAST();
        for (auto &index : *indexList) {
            list->push_back(pAST(index->copy()));
        }
        return new LValAST(ident, list, typeInfo);
    }

    int eval() const override {
        // TODO support const var
        cerr << "error: lval is not const" << endl;
        assert(false);
        return -1;
    }
    // type for the ident, generated by := stmt
    // indexList not considered
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
        if (indexList != nullptr) {
            if (indexList->size() > 0) {
                j["indexList"] = json::array();
                for (auto &index : *indexList) {
                    j["indexList"].push_back(index->toJson());
                }
            }
        }
        if (typeInfo != "") {
            j["typeInfo"] = typeInfo;
        }
        return j;
    }
};

/**
 * @brief AST of a if statement
 */
class IfStmtAST : public StmtAST {
   public:
    TType ty = TType::IfStmtT;
    enum Type { If, IfElse, IfElseIf } t;
    pAST init = nullptr;
    pAST cond;  // cannot be nullptr
    pAST body;  // cannot be nullptr
    // else block or another if stmt
    pAST elseBlockStmt = nullptr;

    IfStmtAST() = delete;
    IfStmtAST(Type _t, pT _init, pT _cond, pT _body, pT _elseBlockStmt)
        : t(_t) {
        if (_init != nullptr) {
            init = pAST(_init);
        }
        cond = pAST(_cond);
        body = pAST(_body);
        if (_elseBlockStmt != nullptr) {
            elseBlockStmt = pAST(_elseBlockStmt);
        }
        // set parent
        if (init != nullptr) {
            init->setParent(this);
        }
        cond->setParent(this);
        body->setParent(this);
        if (elseBlockStmt != nullptr) {
            elseBlockStmt->setParent(this);
        }
    }

    TType type() const override { return ty; }
    json toJson() const override {
        if (yydebug) cout << "IfStmtAST::toJson" << endl;
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

/**
 * @brief AST of a expression statement
 * @note Example: f(1, 2);
 */
class ExpStmtAST : public StmtAST {
   public:
    TType ty = TType::ExpStmtT;
    pAST exp;

    ExpStmtAST(pT exp) : exp(pAST(exp)) { exp->setParent(this); }

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ExpStmtAST";
        j["exp"] = exp->toJson();
        return j;
    }
};

/**
 * @brief AST of a branch statement
 * @note Example: break; continue; goto label;
 */
class BranchStmtAST : public StmtAST {
   public:
    TType ty = TType::BranchStmtT;
    enum Type { Break, Continue, Goto } t;
    string ident = "";

    BranchStmtAST(Type _t, string *_ident = nullptr) : t(_t) {
        if (_ident != nullptr) {
            ident = *pStr(_ident);
        } else {
            ident = "";
        }
    }

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

/**
 * @brief AST of a inc/dec statement
 * @note Example: i++; i--;
 */
class IncDecStmtAST : public StmtAST {
   public:
    TType ty = TType::IncDecStmtT;
    pAST target;
    bool isInc;

    IncDecStmtAST(pT target, bool isInc) : target(pAST(target)), isInc(isInc) {
        target->setParent(this);
    }

    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "IncDecStmtAST";
        j["target"] = target->toJson();
        j["isInc"] = isInc;
        return j;
    }
};

/**
 * @brief AST of a make expression
 * @note Example: make([][]int, 10)
 * @note This is actually a initValAST not expAST
 */
class MakeExpAST : public ExpAST {
   public:
    TType ty = TType::MakeExpT;
    pAST t;    // BType
    pAST len;  // Exp

    MakeExpAST(BaseAST *tt, BaseAST *exp) {
        t = pAST(tt);
        len = pAST(exp);
        // set parent
        t->setParent(this);
        len->setParent(this);
    }

    int eval() const override {
        cerr << "eval: make exp cannot be const" << endl;
        assert(false);
        return -1;
    }
    BaseAST *copy() const override {
        return new MakeExpAST(t->copy(), len->copy());
    }
    string info() const override { return t->info(); }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "MakeExpAST";
        j["t"] = t->toJson();
        if (len != nullptr) {
            j["len"] = len->toJson();
        }
        return j;
    }
};

/**
 * @brief AST of a array expression
 * @note Example: []int{1, 2, 3}
 * @note This is actually a initValAST not expAST
 */
class ArrayExpAST : public ExpAST {
   public:
    TType ty = TType::ArrayExpT;
    pAST t;              // BType, array type like []int
    pvpAST initValList;  // list of initVal, cannot be nullptr

    ArrayExpAST() = delete;
    ArrayExpAST(pT _t, pvpT list) {
        t = pAST(_t);
        initValList = pvpAST(list);
        // set parent
        t->setParent(this);
        for (auto &exp : *initValList) {
            exp->setParent(this);
        }
    }

    int eval() const override {
        cerr << "eval: array exp cannot be int" << endl;
        assert(false);
        return -1;
    }
    BaseAST *copy() const override {
        auto _t = t->copy();
        auto _list = new vector<pAST>();
        for (auto &exp : *_list) {
            _list->push_back(pAST(exp->copy()));
        }
        auto ret = new ArrayExpAST(_t, _list);
        return ret;
    }
    string info() const override { return t->info(); }
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "ArrayExpAST";
        j["t"] = t->toJson();
        if (initValList != nullptr) {
            j["initValList"] = json::array();
            for (auto &exp : *initValList) {
                j["initValList"].push_back(exp->toJson());
            }
        }
        return j;
    }
};

/**
 * @brief AST of a nil expression
 * @note The type of nil is nil, which matches any pointer
 */
class NilAST : public ExpAST {
   public:
    TType ty = TType::NilT;

    int eval() const override {
        // TODO maybe enable eval ptr
        cerr << "eval: nil cannot be int" << endl;
        assert(false);
        return -1;
    }
    BaseAST *copy() const override { return new NilAST(); }
    string info() const override { return "nil"; }  // matches any pointer
    TType type() const override { return ty; }
    json toJson() const override {
        json j;
        j["type"] = "NilAST";
        return j;
    }
};