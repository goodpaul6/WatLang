#include <string>
#include <vector>

struct Func;

struct Var
{
    Pos pos;
    std::string name;

    Func* func;
    int loc;    // Initialized to -1; could store register index or memory location as determined by compiler
};

struct Func
{
    Pos pos;
    std::string name;

    std::vector<Var> args;
    std::vector<Var> locals;

    int firstReg; // First unused register (after registers for arguments and locals have been allocated), -1 by default, assigned by compiler
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

        funcs.emplace_back(Func{std::move(pos), std::move(name), {}, {}, -1});
        return funcs.back();
    }

    Var& declArg(Pos pos, std::string name, Func& func)
    {
        for(auto& v : func.args) {
            if(v.name == name) {
                throw PosError{pos, "Multiple declarations of argument " + name};
            }
        }

        func.args.emplace_back(Var{pos, std::move(name), &func, -1});
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

            func->locals.emplace_back(Var{pos, std::move(name), func, -1});

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

private:
    friend struct Compiler;

    std::vector<Var> globals;
    std::vector<Func> funcs;
    std::vector<CString> strings;
};
