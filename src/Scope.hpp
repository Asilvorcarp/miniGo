#pragma once

#include <AST.hpp>

using namespace std;

/**
 * @brief The Object class represents a named object in the scope
 * @note The name is the original name of the object, while the mangled name is
 * the name used in the LLVM IR
 */
class Object {
   public:
    /**
     * @brief the original name
    */
    string Name;
    /**
     * @brief the mangled name used in the LLVM IR
    */
    string MangledName;
    /**
     * @brief The AST node of the object
     * @note The AST node is used to get the type information of the object,
     * which can be a FuncDefAST, VarSpecAST, or LValAST
     */
    BaseAST* Node;

    Object() = delete;
    /**
     * @brief Construct a new Object object
     * 
     * @param name the original name
     * @param mangledName the mangled name used in the LLVM IR
     * @param node the AST node of the object to get the type information
     */
    Object(string name, string mangledName, BaseAST* node)
        : Name(name), MangledName(mangledName), Node(node) {}
};

/**
 * @brief A scope in the program
 * @note The scope is a tree structure, and the root is the universe scope
 * @note The universe scope contains all the runtime functions
 */
class Scope {
   public:
    /**
    * @brief The outer scope of the current scope
    */
    Scope* Outer;
    /**
     * @brief The map from the name to the object
     * @note The name is the original name of the object, while the mangled name
     * is the name used in the LLVM IR
     */
    map<string, Object*> Objs;

    /**
     * @brief Construct a new Scope object
     * 
     * @param outer the outer scope of the current scope
     */
    Scope(Scope* outer) : Outer(outer) { Objs = map<string, Object*>(); }

    /**
     * @brief Check if the scope has the object with the given name
     * 
     * @param name the name of the object
     * @return true 
     * @return false 
     */
    bool HasName(string name) { return Objs.find(name) != Objs.end(); }

    /**
     * @brief Lookup the object with the given name in the scope
     * 
     * @param name the name of the object
     * @return pair<Scope*, Object*> the scope containing the object and the object
     */
    pair<Scope*, Object*> Lookup(string name) {
        if (HasName(name)) {
            return make_pair(this, Objs[name]);
        }
        if (Outer != nullptr) {
            return Outer->Lookup(name);
        }
        return make_pair(nullptr, nullptr);
    }

    /**
     * @brief Insert the object into the scope
     * 
     * @param obj the object to be inserted
     * @return Object* - the object with the same name in the scope, or nullptr if
     * no object with the same name exists
     */
    Object* Insert(Object* obj) {
        if (HasName(obj->Name)) {
            return Objs[obj->Name];
        }
        Objs[obj->Name] = obj;
        return nullptr;
    }

    /**
     * @brief Convert the scope to json
     * 
     * @return json
     */
    json toJson() const {
        json j;
        j["type"] = "Scope";
        j["Objs"] = json::array();
        for (auto& obj : Objs) {
            j["Objs"].push_back(obj.second->Node->toJson());
        }
        return j;
    }

    /**
     * @brief Convert the scope to json string
     * 
     * @return string 
     */
    friend ostream& operator<<(ostream& os, const Scope& s) {
        return os << s.toJson().dump(4);
    }

    /**
     * @brief Get the universe scope, which contains all the runtime functions
     * 
     * @return Scope* - the universe scope
     */
    static Scope* Universe() {
        auto universe = new Scope(nullptr);
        // --- Add Runtime Functions Here ---
        // no malloc because it is called by make()
        universe->Insert(
            new Object("getchar", "@getchar",
                       new RuntimeFuncAST("i64", new vector<string>())));
        universe->Insert(
            new Object("putchar", "@putchar",
                       new RuntimeFuncAST("i64", new vector<string>{"i64"})));
        return universe;
    }
};

/**
 * @brief The header of the generated LLVM IR, including the runtime functions
 * @note The runtime functions are:
 * - i8* @malloc(i64)
 * - i64 @getchar()
 * - i64 @putchar(i64)
 * @note here, i8* stands for void* or opaque pointer
 */
const static string Header = R"(
target triple = "x86_64-pc-linux-gnu"

declare i8* @malloc(i64)
declare i64 @getchar()
declare i64 @putchar(i64)
)";

/**
 * @brief The fake main function of the generated LLVM IR
 * @note main_init() is called before main_main() to initialize the globals
 */
const string MainMain = R"(
define i64 @main() {
	call void() @main_init()
	call void() @main_main()
	ret i64 0
}
)";
