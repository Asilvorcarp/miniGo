#pragma once

#include <AST.hpp>

using namespace std;

class Object {
   public:
    string Name;
    string MangledName;
    // FuncDefAST, VarSpecAST, LValAST(TODO this now has no type info)
    BaseAST* Node;

    Object() = delete;
    Object(string name, string mangledName, BaseAST* node)
        : Name(name), MangledName(mangledName), Node(node) {}
};

class Scope {
   public:
    Scope* Outer;
    map<string, Object*> Objs;

    Scope(Scope* outer) : Outer(outer) { Objs = map<string, Object*>(); }

    bool HasName(string name) { return Objs.find(name) != Objs.end(); }

    pair<Scope*, Object*> Lookup(string name) {
        if (HasName(name)) {
            return make_pair(this, Objs[name]);
        }
        if (Outer != nullptr) {
            return Outer->Lookup(name);
        }
        return make_pair(nullptr, nullptr);
    }

    Object* Insert(Object* obj) {
        if (HasName(obj->Name)) {
            return Objs[obj->Name];
        }
        Objs[obj->Name] = obj;
        return nullptr;
    }

    json toJson() const {
        json j;
        j["type"] = "Scope";
        j["Objs"] = json::array();
        for (auto& obj : Objs) {
            j["Objs"].push_back(obj.second->Node->toJson());
        }
        return j;
    }

    friend ostream& operator<<(ostream& os, const Scope& s) {
        return os << s.toJson().dump(4);
    }

    static Scope* Universe() {
        auto universe = new Scope(nullptr);
        // --- Add Runtime Functions Here ---
        // no malloc because it is called by make()
        universe->Insert(
            new Object("getchar", "@getchar",
                       new RuntimeFuncAST("i32", new vector<string>())));
        universe->Insert(
            new Object("putchar", "@putchar",
                       new RuntimeFuncAST("i32", new vector<string>{"i32"})));
        return universe;
    }
};

// the runtime functions
const static string Header = R"(
target triple = "x86_64-pc-linux-gnu"

declare ptr @malloc(i32)
declare i32 @getchar()
declare i32 @putchar(i32)
)";

const string MainMain = R"(
define i32 @main() {
	call void() @main_init()
	call void() @main_main()
	ret i32 0
}
)";
