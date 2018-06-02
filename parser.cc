#include <fstream>
#include <memory>
#include <vector>
#include <unordered_set>

class Parser
{
    Lexer lexer;
    std::unordered_set<std::string> includes;

    int curTok = 0;
    Func* curFunc = nullptr;

    void expectToken(int tok, const std::string& message)
    {
        if(curTok != tok) {
            throw PosError{lexer.getPos(), message};
        }
    }

    void eatToken(std::istream& s, int tok, const std::string& message)
    {
        expectToken(tok, message);
        curTok = lexer.getToken(s);
    }

    const Typetag* parseType(SymbolTable& table, std::istream& s)
    {
        if(curTok == '*') {
            curTok = lexer.getToken(s);

            auto inner = parseType(table, s);

            return table.getPtrTag(inner);
        } else {
            expectToken(TOK_ID, "Expected a type identifier.");

            const Typetag* tag = nullptr;
            
            if(lexer.getLexeme() == "int") {
                tag = table.getPrimTag(Typetag::INT);
            } else if(lexer.getLexeme() == "char") {
                tag = table.getPrimTag(Typetag::CHAR);
            } else if(lexer.getLexeme() == "bool") { 
                tag = table.getPrimTag(Typetag::BOOL);
            } else if(lexer.getLexeme() == "void") {
                tag = table.getPrimTag(Typetag::VOID);
            } else {
                throw PosError{lexer.getPos(), lexer.getLexeme() + " does not denote a typename."};
            }

            curTok = lexer.getToken(s);

            return tag;
        }
    }

    std::unique_ptr<AST> parseCall(Pos pos, SymbolTable& table, std::string funcName, std::istream& s)
    {
        // Function call (we will check if the function exists during compilation)
        eatToken(s, '(', "Expected '(' after function name.");

        std::vector<std::unique_ptr<AST>> args;

        while(curTok != ')') {
            args.emplace_back(parseExpr(table, s));

            if(curTok == ',') {
                curTok = lexer.getToken(s);
            } else if(curTok != ')') {
                throw PosError{lexer.getPos(), "Expected ',' or ')' in argument list."};
            }
        }
        
        // HACK(Apaar): We automatically pass in extra arguments to any function called "assert".
        if(funcName == "assert") {
            int id = table.internString(lexer.getPos().filename);
            args.emplace_back(new StrAST{pos, id});
            args.emplace_back(new IntAST{pos, pos.line, AST::INT});
        }

        curTok = lexer.getToken(s);

        return std::unique_ptr<AST>{new CallAST{pos, std::move(funcName), std::move(args)}};
    }
    
    std::unique_ptr<AST> parseUnary(SymbolTable& table, std::istream& s)
    {
        std::unique_ptr<AST> lhs;

        if(curTok == '-' || curTok == '*') {
            auto pos = lexer.getPos();

            int op = curTok;

            curTok = lexer.getToken(s);

            lhs.reset(new UnaryAST{pos, parseUnary(table, s), op});
        } else if(curTok == '(') {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            auto inner = parseExpr(table, s);

            eatToken(s, ')', "Expected ')' to match previous '('.");

            lhs.reset(new ParenAST{pos, std::move(inner)});
        } else if(curTok == TOK_CAST) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            eatToken(s, '(', "Expected '(' after cast.");

            auto type = parseType(table, s);

            eatToken(s, ')', "Expected ')' to match previous '('");

            lhs.reset(new CastAST{pos, parseUnary(table, s), type});
        } else if(curTok == TOK_INT) {
            lhs.reset(new IntAST{lexer.getPos(), lexer.getInt(), AST::INT});
            curTok = lexer.getToken(s);
        } else if(curTok == TOK_CHAR) {
            lhs.reset(new IntAST{lexer.getPos(), lexer.getLexeme()[0], AST::CHAR});
            curTok = lexer.getToken(s);
        } else if(curTok == TOK_STR) {
            int id = table.internString(lexer.getLexeme());
            lhs.reset(new StrAST{lexer.getPos(), id});

            curTok = lexer.getToken(s);
        } else if(curTok == TOK_ID) {
            auto pos = lexer.getPos();
            auto name = lexer.getLexeme();

            curTok = lexer.getToken(s);

            if(curTok != '(') {
                auto var = table.getVar(name, curFunc);
                
                if(!var) {
                    throw PosError{pos, "Referenced undeclared variable " + name};
                }  

                lhs.reset(new IdAST{pos, std::move(name)});
            } else { 
                lhs = parseCall(pos, table, std::move(name), s);
            }
        } else if(curTok == '[') {
            auto pos = lexer.getPos();

            curTok = lexer.getToken(s);

            int length = -1;

            if(curTok == ']') {
                curTok = lexer.getToken(s);
            } else {
                expectToken(TOK_INT, "Expected integer or ']' after '['.");

                length = lexer.getInt();

                curTok = lexer.getToken(s);

                eatToken(s, ']', "Expected ']' after '[' and integer.");
            }

            if(curTok == '{') {
                curTok = lexer.getToken(s);

                std::vector<int> values;

                while(curTok != '}') {
                    int fac = 1;

                    if(curTok == '-') {
                        fac = -1;
                        curTok = lexer.getToken(s);
                    }

                    expectToken(TOK_INT, "Expected integer in array literal.");

                    values.emplace_back(fac * lexer.getInt());

                    curTok = lexer.getToken(s);

                    if(curTok == ',') {
                        curTok = lexer.getToken(s);
                    } else if(curTok != '}') {
                        throw PosError{pos, "Expected ',' or '}' in array literal."};
                    }
                }

                curTok = lexer.getToken(s);

                lhs.reset(new ArrayAST{pos, length, std::move(values), AST::ARRAY});
            } else {
                expectToken(TOK_STR, "Expected string or '{' after '['.");

                std::vector<int> values;
                values.reserve(lexer.getLexeme().size());

                for(auto ch : lexer.getLexeme()) {
                    values.push_back(ch);
                }
                values.push_back(0);

                lhs.reset(new ArrayAST{pos, length, std::move(values), AST::ARRAY_STRING});

                curTok = lexer.getToken(s);
            }
        } else if(curTok == TOK_TRUE || curTok == TOK_FALSE) {
            lhs.reset(new IntAST{lexer.getPos(), curTok == TOK_TRUE, AST::BOOL});
            curTok = lexer.getToken(s);
        } else {
            throw PosError{lexer.getPos(), "Unexpected token."};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseFactor(SymbolTable& table, std::istream& s)
    {
        auto lhs = parseUnary(table, s);
        
        while(curTok == '*' || curTok == '/') {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseFactor(table, s);

            lhs = std::unique_ptr<AST>{new BinAST{lexer.getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseTerm(SymbolTable& table, std::istream& s)
    {
        auto lhs = parseFactor(table, s);

        while(curTok == '+' || curTok == '-') {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseTerm(table, s);

            lhs = std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseRelation(SymbolTable& table, std::istream& s)
    {
        auto lhs = parseTerm(table, s);

        // Unlike terms, relations are limited to a single binary expression
        if(curTok == '<' || curTok == '>' || curTok == TOK_EQUALS || curTok == TOK_LTE || curTok == TOK_GTE || curTok == TOK_NOTEQUALS) {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseTerm(table, s);

            lhs = std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseExpr(SymbolTable& table, std::istream& s)
    {
        auto lhs = parseRelation(table, s);

        while(curTok == TOK_LOGICAL_AND || curTok == TOK_LOGICAL_OR) {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseRelation(table, s);

            lhs = std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }
 
    std::unique_ptr<AST> parseStatement(SymbolTable& table, std::istream& s)
    {
        if(!curFunc) {
            if(curTok != TOK_FUNC && curTok != TOK_VAR && curTok != TOK_DIRECTIVE) {
                throw PosError{lexer.getPos(), "Unexpected top-level token near " + lexer.getLexeme()};
            }
        }

        if(curTok == TOK_STRUCT) {
            if(curFunc) {
                throw PosError{lexer.getPos(), "Can't declare structs inside a function. Sorry."};
            }

            // TODO(Apaar): Implement this
            assert(0);
        } else if(curTok == '{') {
            auto pos = lexer.getPos();

            std::vector<std::unique_ptr<AST>> asts;

            curTok = lexer.getToken(s);

            while(curTok != '}') {
                auto ast = parseStatement(table, s);
                asts.emplace_back(std::move(ast)); 
            }

            curTok = lexer.getToken(s);

            return std::unique_ptr<AST>{new BlockAST{pos, std::move(asts)}};
        } else if(curTok == '*') {
            auto pos = lexer.getPos();

            auto lhs = parseUnary(table, s);

            eatToken(s, '=', "Expected '=' after unary expression.");

            auto rel = parseExpr(table, s);

            eatToken(s, ';', "Expected ';' after statement.");

            return std::unique_ptr<AST>{new BinAST{pos, std::move(lhs), std::move(rel), '='}};
        } else if(curTok == TOK_VAR || curTok == TOK_ID) {
            auto pos = lexer.getPos();

            std::string name;

            Var* decl = nullptr;

            if(curTok == TOK_VAR) {
                curTok = lexer.getToken(s);

                expectToken(TOK_ID, "Expected identifier after 'var'.\n");

                pos = lexer.getPos();

                decl = &table.declVar(lexer.getPos(), lexer.getLexeme(), curFunc);
                name = lexer.getLexeme();
            } else {
                name = lexer.getLexeme();
            }

            std::unique_ptr<AST> lhs{new IdAST{lexer.getPos(), lexer.getLexeme()}};

            curTok = lexer.getToken(s);

            if(decl) {
                eatToken(s, ':', "Expected ':' after var " + name);

                decl->typetag = parseType(table, s);
            }

            if(!curFunc) {
                if(curTok == '=') {
                    throw PosError{lexer.getPos(), "Top level assignment expressions are not allowed."};
                } else {
                    if(!curFunc) {
                        // HACK(APAAR): We do this because we don't want to
                        // be inconsistent about semicolons at the top-level.
                        // Since this code isn't inside a block, it wouldn't
                        // automatically be eaten.
                        eatToken(s, ';', "Expected ';' after var decl.");
                    }

                    return parseStatement(table, s);
                }
            }

            if(decl) {
                expectToken('=', "Expected '=' after var ...");
            }

            if(curTok == '=') {
                curTok = lexer.getToken(s);

                auto rhs = parseTerm(table, s);

                eatToken(s, ';', "Expected ';' after var decl.");

                return std::unique_ptr<AST>{new BinAST{pos, std::move(lhs), std::move(rhs), '='}};
            } else if(curTok != '(') {
                throw PosError{lexer.getPos(), "Expected call or assignment statement."};
            }

            auto call = parseCall(pos, table, std::move(name), s);

            eatToken(s, ';', "Expected ';' after call.");

            return call;
        } else if(curTok == TOK_IF) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            eatToken(s, '(', "Expected '(' after 'if'.");

            auto cond = parseExpr(table, s);

            eatToken(s, ')', "Expected ')' after 'if'.");

            auto body = parseStatement(table, s);

            std::unique_ptr<AST> alt;

            if(curTok == TOK_ELSE) {
                curTok = lexer.getToken(s);

                alt = parseStatement(table, s);
            }

            return std::unique_ptr<AST>{new IfAST{pos, std::move(cond), std::move(body), std::move(alt)}};
        } else if(curTok == TOK_WHILE) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            eatToken(s, '(', "Expected '(' after 'while'.");

            auto cond = parseExpr(table, s);

            eatToken(s, ')', "Expected ')' after 'while'.");

            auto body = parseStatement(table, s);

            return std::unique_ptr<AST>{new WhileAST{pos, std::move(cond), std::move(body)}};
        } else if(curTok == TOK_FUNC) {
            auto pos = lexer.getPos();
            if(curFunc) { 
                throw PosError{pos, "Cannot declare function inside function."};
            }

            curTok = lexer.getToken(s);

            expectToken(TOK_ID, "Expected identifier after 'func'.");

            curFunc = &table.declFunc(pos, lexer.getLexeme());

            auto funcName = curFunc->name;
            
            curTok = lexer.getToken(s);

            eatToken(s, '(', "Expected '(' after " + curFunc->name);

            while(curTok != ')') {
                expectToken(TOK_ID, "Expected identifier in argument list.\n");

                auto& var = table.declArg(lexer.getPos(), lexer.getLexeme(), *curFunc);

                curTok = lexer.getToken(s);

                eatToken(s, ':', "Expected : after argument.");

                var.typetag = parseType(table, s);

                if(curTok == ',') {
                    curTok = lexer.getToken(s);
                } else if(curTok != ')') {
                    throw PosError{lexer.getPos(), "Expected ',' or ')' after arg."};
                }
            }

            curTok = lexer.getToken(s);

            eatToken(s, ':', "Expected ':' after function prototype.");

            curFunc->returnType = parseType(table, s);

            auto body = parseStatement(table, s);

            curFunc = nullptr;

            return std::unique_ptr<AST>{new FuncAST{pos, std::move(funcName), std::move(body)}};
        } else if(curTok == TOK_RETURN) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            if(curTok == ';') {
                // No return value
                curTok = lexer.getToken(s);

                return std::unique_ptr<AST>{new ReturnAST{pos, nullptr}};
            } else {
                auto val = parseExpr(table, s);

                eatToken(s, ';', "Expected ';' after return expression.");

                return std::unique_ptr<AST>{new ReturnAST{pos, std::move(val)}};
            }
        } else if(curTok == TOK_ASM) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            expectToken(TOK_STR, "Expected string after 'asm'.");

            auto str = lexer.getLexeme();
            curTok = lexer.getToken(s);
            
            eatToken(s, ';', "Expected ';' after asm string.");

            return std::unique_ptr<AST>{new AsmAST{pos, std::move(str)}};
        } else if(curTok == TOK_DIRECTIVE) {
            auto pos = lexer.getPos();
            if(lexer.getLexeme() == "include") {
                if(curFunc) {
                    throw PosError{pos, "Cannot put #include inside function"};
                }

                curTok = lexer.getToken(s);

                expectToken(TOK_STR, "Expected string after '#include'.");
                
                // We've already included this file, so don't bother
                if(includes.find(lexer.getLexeme()) != includes.end()) {
                    curTok = lexer.getToken(s);
                    return parseStatement(table, s);
                }

                auto filename = lexer.getLexeme();

                std::ifstream f{filename};
                curTok = lexer.getToken(s);

                Parser p;

                p.includes = includes;

                auto asts = p.parseUntilEof(table, f, filename);

                // Merge the includes from the included file
                for(auto& i : p.includes) {
                    includes.insert(i);
                }

                return std::unique_ptr<AST>{new BlockAST{pos, std::move(asts)}};
            } else {
                throw PosError{pos, "Invalid directive #" + lexer.getLexeme()};
            }
        } else {
            throw PosError{lexer.getPos(), "Unexpected token (" + std::to_string(curTok) + ") near " + lexer.getLexeme()};
        }
    }

public:
    std::vector<std::unique_ptr<AST>> parseUntilEof(SymbolTable& table, std::istream& s, const std::string& filename = "")
    {
        lexer = {};

        lexer.setPos({1, filename});

        std::vector<std::unique_ptr<AST>> asts;

        curTok = lexer.getToken(s);

        while(curTok != TOK_EOF) {
            auto ast = parseStatement(table, s);
            asts.emplace_back(std::move(ast));
        }

        return asts;
    }
};

