#include <istream>
#include <memory>
#include <vector>

struct Parser
{
    std::vector<std::unique_ptr<AST>> parseUntilEof(Lexer& lexer)
    {
        std::vector<std::unique_ptr<AST>> asts;

        curTok = lexer.getToken();

        while(curTok != TOK_EOF) {
            asts.emplace_back(std::move(parseStatement(lexer)));
        }

        return asts;
    }

private:
    int curTok = 0;

    std::unique_ptr<AST> parseFactor(Lexer& lexer)
    {
    }
};

