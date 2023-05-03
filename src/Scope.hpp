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
        universe->Insert(new Object("println", "@ugo_builtin_println"));
        universe->Insert(new Object("exit", "@ugo_builtin_exit"));
        return universe;
    }
};
