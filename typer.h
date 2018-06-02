#pragma once

#include <cassert>
#include <string>
#include <vector>

struct Typetag
{
    enum Tag
    {
        VOID,
        BOOL,
        CHAR,
        INT,
        PTR,
        STRUCT
    } tag;

    const Typetag* inner;

    using Fields = std::vector<std::pair<std::string, const Typetag*>>;

    std::string structName;
    Fields structFields;
    Pos structDeclPos;

    explicit Typetag(Tag tag, const Typetag* inner = nullptr) :
        tag{tag}, inner{inner} {}

    explicit Typetag(Pos pos, std::string name) : tag{STRUCT}, structName{std::move(name)}, structDeclPos{std::move(pos)} {}

    operator std::string() const
    {
        switch(tag) {
            case VOID: return "void";
            case BOOL: return "bool";
            case CHAR: return "char";
            case INT: return "int";
            case PTR: return "*" + static_cast<std::string>(*inner);
            case STRUCT: return structName;
            default: assert(0); return "ERROR"; break;
        }
    }

    int getSizeInWords() const
    {
        switch(tag) {
            case VOID: assert(0); return 0; break;

            case STRUCT: {
                int totalSize = 0;

                for(auto& field : structFields) {
                    totalSize += field.second->getSizeInWords();
                }

                return totalSize;
            } break;

            default: return 1;
        }
    }
};
