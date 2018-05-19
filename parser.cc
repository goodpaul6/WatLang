#include <istream>
#include <memory>
#include <vector>

class Parser
{
    Lexer lexer;
    int curTok = 0;

    void expectToken(int tok, const std::string& message)
    {
        if(curTok != tok) {
            throw PosError{pos, message};
        }
    }

    void eatToken(std::istream& s, int tok, const std::string& message)
    {
        expectToken(tok, message);
        curTok = lexer.getToken(s);
    }

    std::unique_ptr<AST> parseFactor(std::istream& s)
    {
        expectToken(TOK_INT, "Expected integer.");

        auto lhs = std::make_unique<IntAST>(lexer.getPos(), lexer.getInt());

        while(curTok == '*' || curTok == '/') {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseFactor(s);

            lhs = std::make_unique<BinAST>(lexer.getPos(), std::move(lhs), std::move(rhs), op);
        }

        return lhs;
    }

    std::unique_ptr<AST> parseTerm(std::istream& s)
    {
        auto lhs = parseFactor(s);

        while(curTok == '+' || curTok == '/') {
            int op = curTok;

            curTok = lexer.getToken(s);

            auto rhs = parseTerm(s);

            lhs = std::make_unique<BinAST>(lexer.getPos(), std::move(lhs), std::move(rhs), op);
        }

        return lhs;
    }

    std::unique_ptr<AST> parseStatement(std::istream& s)
    {
        expectToken(TOK_ID, "Expected identifier.");

        auto pos = lexer.getPos();
        auto lhs = std::make_unique<IdAST>(lexer.getPos(), lexer.getLexeme());

        curTok = lexer.getToken(s);

        expectToken('=', "Expected '=' after identifier.");

        curTok = lexer.getToken(s);

        auto rhs = parseTerm(s);

        return std::make_unique<BinAST>(pos, lhs, rhs, '=');
    }

public:
    std::vector<std::unique_ptr<AST>> parseUntilEof(std::istream& s)
    {
        lexer = {};

        std::vector<std::unique_ptr<AST>> asts;

        curTok = lexer.getToken(s);

        while(curTok != TOK_EOF) {
            asts.emplace_back(std::move(parseStatement(s)));
        }

        return asts;
    }
};

