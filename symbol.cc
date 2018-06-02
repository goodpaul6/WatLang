#include <string>
#include <vector>
#include <memory>

#include "typer.h"

struct Func;

struct AST;

struct Var
{
    Pos pos;
    std::string name;

    Func* func;
    int loc;    // Initialized to -1; could store register index or memory location as determined by compiler
    
    const Typetag* typetag;
};

struct Func
{
    Pos pos;
    std::string name;

    std::vector<Var> args;
    std::vector<Var> locals;

    int firstReg; // First unused register (after registers for arguments and locals have been allocated), -1 by default, assigned by compiler

    const Typetag* returnType;
};

struct CString
{
    std::string str;
    int loc;
};

struct SymbolTable
{
    Func& declFunc(Pos pos, std::string name)
    {
        for(auto& f : funcs) {
            if(f.name == name) {
                throw PosError{pos, "Multiple declarations of function " + name};
            }
        }

        funcs.emplace_back(Func{std::move(pos), std::move(name), {}, {}, -1, nullptr});
        return funcs.back();
    }

    Var& declArg(Pos pos, std::string name, Func& func)
    {
        for(auto& v : func.args) {
            if(v.name == name) {
                throw PosError{pos, "Multiple declarations of argument " + name};
            }
        }

        func.args.emplace_back(Var{pos, std::move(name), &func, -1, nullptr});
        return func.args.back();
    }

    Var& declVar(Pos pos, std::string name, Func* func)
    {
        if(func) {
            for(auto& v : func->locals) {
                if(v.name == name) {
                    throw PosError{pos, "Multiple declarations of local var " + name};
                }
            }

            func->locals.emplace_back(Var{pos, std::move(name), func, -1, nullptr});

            return func->locals.back();
        }

        for(auto& v : globals) {
            if(v.name == name) {
                throw PosError{pos, "Multiple declarations of global var " + name};
            }
        }

        globals.emplace_back(Var{pos, std::move(name), nullptr, -1});
        return globals.back();
    }

    Var* getVar(const std::string& name, Func* func)
    {
        if(func) {
            for(auto& v : func->locals) {
                if(v.name == name) {
                    return &v;
                }
            }

            for(auto& v : func->args) {
                if(v.name == name) {
                    return &v;
                }
            }
        }

        for(auto& v : globals) {
            if(v.name == name) {
                return &v;
            }
        }

        return nullptr;
    }

    Func* getFunc(const std::string& name)
    {
        for(auto& f : funcs) {
            if(f.name == name) {
                return &f;
            }
        }

        return nullptr;
    }

    int internString(std::string str)
    {
        auto i = 0u;
        for(auto& s : strings) {
            if(s.str == str) {
                return i;
            }
            i += 1;
        }

        strings.emplace_back(CString{std::move(str), -1});
        return strings.size() - 1;
    }

    const CString& getString(int id)
    {
        return strings[id];
    }

    const Typetag* getPrimTag(Typetag::Tag tag)
    {
        static Typetag v{Typetag::VOID};
        static Typetag b{Typetag::BOOL};
        static Typetag c{Typetag::CHAR};
        static Typetag i{Typetag::INT};

        switch(tag) {
            case Typetag::VOID: return &v;
            case Typetag::BOOL: return &b;
            case Typetag::CHAR: return &c;
            case Typetag::INT: return &i;
            default: assert(0); return nullptr;
        }
    }

    const Typetag* getPtrTag(const Typetag* inner)
    {
        for(auto& tag : tags) {
            if(tag->tag == Typetag::PTR && tag->inner == inner) {
                return tag.get();
            }
        }

        tags.emplace_back(new Typetag{Typetag::PTR, inner});
        return tags.back().get();
    }

    const Typetag* getStruct(Pos pos, const std::string& name)
    {
        // If a type that hasn't been defined yet is referenced, we make a placeholder struct
        // for it which will be filled out if it does get defined. Otherwise, we'll complain about
        // it during the compilation step.
        for(auto& tag : tags) {
            if(tag->tag == Typetag::STRUCT && tag->structName == name) {
                return tag.get();
            }
        }

        tags.emplace_back(new Typetag{std::move(pos), name});
        return tags.back().get();
    }

    const Typetag* defineStruct(Pos pos, const std::string& name, Typetag::Fields fields)
    {
        if(fields.empty()) {
            throw PosError{pos, "Struct definitions must contain at least one field."};
        }

        Typetag* sTag = nullptr;

        for(auto& tag : tags) {
            if(tag->tag == Typetag::STRUCT && tag->structName == name) {
                if(!tag->structFields.empty()) {
                    throw PosError{pos, "Attempted to redefine struct " + name};
                } else {
                    sTag = tag.get();
                    break;
                }
            }
        }

        if(!sTag) {
            tags.emplace_back(new Typetag{std::move(pos), name});
            sTag = tags.back().get();
        }

        sTag->structFields = std::move(fields);

        return sTag;
    }

private:
    friend struct Compiler;

    std::vector<Var> globals;
    std::vector<Func> funcs;
    std::vector<CString> strings;

    std::vector<std::unique_ptr<Typetag>> tags;
};
