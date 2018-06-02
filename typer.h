#pragma once

#include <cassert>

struct Typetag
{
    enum Tag
    {
        VOID,
        BOOL,
        CHAR,
        INT,
        PTR
    } tag;

    const Typetag* inner = nullptr;

    operator std::string() const
    {
        switch(tag) {
            case VOID: return "void";
            case BOOL: return "bool";
            case CHAR: return "char";
            case INT: return "int";
            case PTR: return "*" + static_cast<std::string>(*inner);
            default: assert(0); return "ERROR"; break;
        }
    }
};
