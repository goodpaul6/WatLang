#include <istream>
#include <string>
#include <cctype>

enum Token
{
    TOK_VAR = -1,
    TOK_FUNC = -2,
    TOK_IF = -3,
    TOK_ELSE = -4,
    TOK_WHILE = -5,
    TOK_FOR = -6,
    TOK_RETURN = -7,
    TOK_ID = -8,
    TOK_INT = -9,
    TOK_STR = -10,
    TOK_EOF = -11
};

struct Pos
{
    int line;
};

struct Lexer
{
    Pos getPos() const { return pos; }
    void setPos(Pos pos) { this->pos = std::move(pos); }

    int getToken(std::istream& s)
    {
        using namespace std;

        while(isspace(last)) {
            if(last == '\n') ++pos.line;
            last = s.get();
        }
        
        if(isalpha(last)) {
            lexeme.clear();

            while(isalnum(last) || last == '_') {
                lexeme += last;
                last = s.get();
            }

            if(lexeme == "var") return TOK_VAR;
            if(lexeme == "func") return TOK_FUNC;
            if(lexeme == "if") return TOK_IF;
            if(lexeme == "else") return TOK_ELSE;
            if(lexeme == "while") return TOK_WHILE;
            if(lexeme == "for") return TOK_FOR;
            if(lexeme == "return") return TOK_RETURN;

            return TOK_ID;
        }

        if(isdigit(last)) {
            lexeme.clear();

            while(isdigit(last)) {
                lexeme += last;
                last = s.get();
            }

            return TOK_INT;
        }

        if(last == '"') {
            lexeme.clear();
            last = s.get();

            while(last != '"') {
                lexeme += last;
                last = s.get();
            }

            return TOK_STR;
        }

        if(last == EOF) {
            return TOK_EOF;
        }

        int lastCh = last;
        last = s.get();

        return lastCh;
    }

    const std::string& getLexeme() const { return lexeme; }
    int getInt() const { return std::stoi(lexeme); }

private:
    int last = ' ';
    Pos pos{1};

    std::string lexeme;
};
