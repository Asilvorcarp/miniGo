#pragma once

#include <AST.hpp>

using namespace std;

class Object {
   public:
    string Name;
    string MangledName;
    BaseAST* Node;

    Object(string name, BaseAST* node) : Name(name), Node(node) {}
    Object(string name, string mangledName, BaseAST* node)
        : Name(name), MangledName(mangledName), Node(node) {}
    Object(string name, string mangledName)
        : Name(name), MangledName(mangledName), Node(nullptr) {}
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
        universe->Insert(new Object("getchar", "@runtime_getchar"));
        universe->Insert(new Object("getint", "@runtime_getint"));
        universe->Insert(new Object("putchar", "@runtime_putchar"));
        universe->Insert(new Object("putint", "@runtime_putint"));
        universe->Insert(new Object("println", "@runtime_println"));
        universe->Insert(new Object("exit", "@runtime_exit"));
        return universe;
    }
};

const static string Header = R"(

target triple = "x86_64-pc-linux-gnu"

declare i32 @runtime_getchar()
declare i32 @runtime_getint()
declare i32 @runtime_putchar(i32)
declare i32 @runtime_putint(i32)
declare i32 @runtime_println(i32)
declare i32 @runtime_exit(i32)
)";

const string MainMain = R"(
define i32 @main() {
	call i32() @main_init()
	call i32() @main_main()
	ret i32 0
}
)";
