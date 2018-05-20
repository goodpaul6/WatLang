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

            lhs = std::unique_ptr<AST>{new BinAST{lexer.getPos(), std::move(lhs), std::move(rhs), op}};
        }

        return lhs;
    }

    std::unique_ptr<AST> parseStatement(SymbolTable& table, std::istream& s)
    {
        expectToken(TOK_ID, "Expected identifier.");

        auto pos = lexer.getPos();

        table.declVar(lexer.getPos(), lexer.getLexeme(), nullptr);

        std::unique_ptr<AST> lhs{new IdAST{lexer.getPos(), lexer.getLexeme()}};

        curTok = lexer.getToken(s);

        expectToken('=', "Expected '=' after identifier.");

        curTok = lexer.getToken(s);

        auto rhs = parseTerm(table, s);

        return std::make_unique<BinAST>(pos, std::move(lhs), std::move(rhs), '=');
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

