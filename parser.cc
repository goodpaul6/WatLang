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

    std::unique_ptr<AST> parseFactor(SymbolTable& table, std::istream& s)
    {
        std::unique_ptr<AST> lhs;
        
        if(curTok == TOK_INT) {
            lhs.reset(new IntAST{lexer.getPos(), lexer.getInt()});
        } else if(curTok == TOK_ID) {
            auto var = table.getVar(lexer.getLexeme(), curFunc);
            
            if(!var) {
                throw PosError{lexer.getPos(), "Referenced undeclared variable " + lexer.getLexeme()};
            }
    
            lhs.reset(new IdAST{lexer.getPos(), lexer.getLexeme()});
        } else {
            throw PosError{lexer.getPos(), "Unexpected token."};
        }

        curTok = lexer.getToken(s);

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

        while(curTok == '+' || curTok == '/') {
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
            if(curTok == TOK_VAR) {
                curTok = lexer.getToken(s);

                expectToken(TOK_ID, "Expected identifier after 'var'.\n");

                auto pos = lexer.getPos();

                table.declVar(lexer.getPos(), lexer.getLexeme(), nullptr);
            }

            std::unique_ptr<AST> lhs{new IdAST{lexer.getPos(), lexer.getLexeme()}};

            curTok = lexer.getToken(s);

            expectToken('=', "Expected '=' after identifier.");

            curTok = lexer.getToken(s);

            auto rhs = parseTerm(table, s);

            return std::unique_ptr<AST>{new BinAST{lhs->getPos(), std::move(lhs), std::move(rhs), '='}};
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

