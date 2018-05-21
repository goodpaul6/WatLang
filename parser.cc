#include <istream>
#include <memory>
#include <vector>

class Parser
{
    Lexer lexer;
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

    std::unique_ptr<AST> parseCall(Pos pos, SymbolTable& table, std::string funcName, std::istream& s)
    {
        // Function call (we will check if the function exists during compilation)
        eatToken(s, '(', "Expected '(' after function name.");

        std::vector<std::unique_ptr<AST>> args;

        while(curTok != ')') {
            args.emplace_back(parseRelation(table, s));

            if(curTok == ',') {
                curTok = lexer.getToken(s);
            } else if(curTok != ')') {
                throw PosError{lexer.getPos(), "Expected ',' or ')' in argument list."};
            }
        }

        curTok = lexer.getToken(s);

        return std::unique_ptr<AST>{new CallAST{pos, std::move(funcName), std::move(args)}};
    }

    std::unique_ptr<AST> parseFactor(SymbolTable& table, std::istream& s)
    {
        std::unique_ptr<AST> lhs;
        
        if(curTok == TOK_INT) {
            lhs.reset(new IntAST{lexer.getPos(), lexer.getInt()});
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
        } else {
            throw PosError{lexer.getPos(), "Unexpected token."};
        }

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
        if(curTok == '<' || curTok == '>' || curTok == TOK_EQUALS || curTok == TOK_LTE || curTok == TOK_GTE) {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseTerm(table, s);

            lhs = std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseStatement(SymbolTable& table, std::istream& s)
    {
        if(curTok == '{') {
            auto pos = lexer.getPos();

            std::vector<std::unique_ptr<AST>> asts;

            curTok = lexer.getToken(s);

            while(curTok != '}') {
                asts.emplace_back(std::move(parseStatement(table, s)));
            }

            curTok = lexer.getToken(s);

            return std::unique_ptr<AST>{new BlockAST{pos, std::move(asts)}};
        } else if(curTok == TOK_VAR || curTok == TOK_ID) {
            auto pos = lexer.getPos();

            bool varDecl = false;

            std::string name;

            if(curTok == TOK_VAR) {
                curTok = lexer.getToken(s);

                expectToken(TOK_ID, "Expected identifier after 'var'.\n");

                pos = lexer.getPos();

                table.declVar(lexer.getPos(), lexer.getLexeme(), curFunc);
                name = lexer.getLexeme();

                varDecl = true;
            } else {
                name = lexer.getLexeme();
            }

            std::unique_ptr<AST> lhs{new IdAST{lexer.getPos(), lexer.getLexeme()}};

            curTok = lexer.getToken(s);

            if(varDecl) {
                expectToken('=', "Expected '=' after var " + name);
            }

            if(curTok == '=') {
                curTok = lexer.getToken(s);

                auto rhs = parseTerm(table, s);

                return std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), '='}};
            } else if(curTok != '(') {
                throw PosError{lexer.getPos(), "Expected call or assignment statement."};
            }

            return parseCall(pos, table, std::move(name), s);
        } else if(curTok == TOK_IF) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            eatToken(s, '(', "Expected '(' after 'if'.");

            auto cond = parseRelation(table, s);

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

            eatToken(s, '(', "Expected '(' after 'if'.");

            auto cond = parseRelation(table, s);

            eatToken(s, ')', "Expected ')' after 'if'.");

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

                table.declArg(lexer.getPos(), lexer.getLexeme(), *curFunc);

                curTok = lexer.getToken(s);

                if(curTok == ',') {
                    curTok = lexer.getToken(s);
                } else if(curTok != ')') {
                    throw PosError{lexer.getPos(), "Expected ',' or ')' after arg."};
                }
            }

            curTok = lexer.getToken(s);

            auto body = parseStatement(table, s);

            curFunc = nullptr;

            return std::unique_ptr<AST>{new FuncAST{pos, std::move(funcName), std::move(body)}};
        } else if(curTok == TOK_RETURN) {
            auto pos = lexer.getPos();
            curTok = lexer.getToken(s);

            if(curTok == ';') {
                curTok = lexer.getToken(s);
                // No return value

                return std::unique_ptr<AST>{new ReturnAST{pos, nullptr}};
            } else {
                return std::unique_ptr<AST>{new ReturnAST{pos, parseRelation(table, s)}};
            }
        } else {
            throw PosError{lexer.getPos(), "Unexpected token near " + lexer.getLexeme()};
        }
    }

public:
    std::vector<std::unique_ptr<AST>> parseUntilEof(SymbolTable& table, std::istream& s)
    {
        lexer = {};

        std::vector<std::unique_ptr<AST>> asts;

        curTok = lexer.getToken(s);

        while(curTok != TOK_EOF) {
            asts.emplace_back(std::move(parseStatement(table, s)));
        }

        return asts;
    }
};

