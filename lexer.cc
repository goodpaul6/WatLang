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
    TOK_EQUALS = -11,
    TOK_LTE = -12,
    TOK_GTE = -13,
    TOK_ASM = -14,
    TOK_DIRECTIVE = -15,
    TOK_CHAR = -16,
    TOK_CAST = -17,
    TOK_NOTEQUALS = -18,
    TOK_TRUE = -20,
    TOK_FALSE = -21,
    TOK_LOGICAL_AND = -22,
    TOK_LOGICAL_OR = -23,
    TOK_EOF = -24
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
            if(lexeme == "asm") return TOK_ASM;
            if(lexeme == "cast") return TOK_CAST;
            if(lexeme == "true") return TOK_TRUE;
            if(lexeme == "false") return TOK_FALSE;

            return TOK_ID;
        }

        if(isdigit(last)) {
            lexeme.clear();

            if(last == '0') {
                lexeme += last;
                last = s.get();

                if(last == 'x') {
                    lexeme += last;
                    last = s.get();
                } 
            }

            while(isxdigit(last)) {
                lexeme += last;
                last = s.get();
            }

            intVal = static_cast<int64_t>(std::strtoll(lexeme.c_str(), nullptr, 0));

            return TOK_INT;
        }

        if(last == '\'') {
            last = s.get();

            lexeme.clear();
            lexeme.push_back(last);

            last = s.get();

            if(last != '\'') {
                throw PosError{pos, "Expected ' to match previous '."};
            }
            
            last = s.get();

            return TOK_CHAR;
        }

        if(last == '"') {
            lexeme.clear();
            last = s.get();

            while(last != '"') {
                lexeme += last;
                last = s.get();
            }
            last = s.get();
            
            return TOK_STR;
        }

        if(last == '#') {
            last = s.get();

            lexeme.clear();
            while(isalpha(last)) {
                lexeme += last;
                last = s.get();
            }

            return TOK_DIRECTIVE;
        }

        if(last == EOF) {
            return TOK_EOF;
        }

        int lastCh = last;
        last = s.get();

        if(lastCh == '=' && last == '=') {
            last = s.get();
            return TOK_EQUALS;
        }

        if(lastCh == '!' && last == '=') {
            last = s.get();
            return TOK_NOTEQUALS;
        }

        if(lastCh == '<' && last == '=') {
            last = s.get();
            return TOK_LTE;
        }

        if(lastCh == '>' && last == '=') {
            last = s.get();
            return TOK_GTE;
        }

        if(lastCh == '|' && last == '|') {
            last = s.get();
            return TOK_LOGICAL_OR;
        }

        if(lastCh == '&' && last == '&') {
            last = s.get();
            return TOK_LOGICAL_AND;
        }

        if(lastCh == '/' && last == '/') {
            while(last != '\n') {
                last = s.get();
            }

            last = s.get();
            pos.line += 1;
            return getToken(s);
        }

        return lastCh;
    }

    const std::string& getLexeme() const { return lexeme; }
    int64_t getInt() const { return intVal; }

private:
    int last = ' ';
    Pos pos{1};

    std::string lexeme;
    int64_t intVal;
};
