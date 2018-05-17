#include <assert.h>
#include <vector>
#include <iostream>
#include <string>

// Reserved registers
// $1-$26 - Variables A to Z
// $27-29 - Temporary regs

namespace
{  
    std::string UniqueLabel()
    {
        static int numLabels = 0;

        static char buf[128];
        sprintf(buf, "L%d", numLabels);

        numLabels += 1;

        return buf;
    }

    void PushRegs(const std::vector<int>& regs)
    {
        using namespace std;

        assert(regs.size() > 0);

        int off = 0;

        for(auto reg : regs) {
            off -= 4;
            cout << "sw $" << reg << ", " << off << "($30)\n";
        }

        cout << "lis $" << regs[0] << "\n";
        cout << ".word " << -off << "\n";
        cout << "sub $30, $30, $" << regs[0] << "\n";
    }

    void PopRegs(const std::vector<int>& regs)
    {
        using namespace std;

        assert(regs.size() > 0);

        int off = regs.size() * -4;

        cout << "lis $" << regs[0] << "\n";
        cout << ".word " << -off << "\n";
        cout << "add $30, $30, $" << regs[0] << "\n";

        for(auto it = regs.rbegin(); it != regs.rend(); ++it) {
            cout << "lw $" << *it << ", " << off << "($30)\n";
            off += 4;
        }
    }

    bool ReadString(std::string& buf)
    { 
        using namespace std;

        char ch;

        if(!(cin >> ch)) {
            cerr << "Expected '\"'.\n";
            return false;
        }

        if(ch != '"') {
            cerr << "Expected '\"'.\n";
            return false;
        }

        buf.clear();

        cin >> noskipws >> ch;

        while(ch != '"') {
            buf += ch;
            cin >> ch;
        }

        return true;
    }

    void CompileArith(std::string& tok, int reg)
    {
        using namespace std;

        if(tok.size() == 1 && tok[0] >= 'A' && tok[0] <= 'Z') {
            cout << "add $" << reg << ", $" << (tok[0] - 'A' + 1) << ", $0\n";
        } else {
            cout << "lis $" << reg << "\n";
            cout << ".word " << stoi(tok) << "\n"; 
        }

        // TODO(Apaar): Support arbitrary whitespace between operand and operator
        cin >> tok;

        if(tok == "+" || tok == "-") {
            char op = tok[0];

            cin >> tok;

            CompileArith(tok, reg + 1);
            cout << (op == '+' ? "add" : "sub") << " $" << reg << ", $" << reg << ", $" << (reg + 1) << "\n";
        } else {
            return;
        }
    }

    void CompilePrint(std::string& tok)
    {
        using namespace std;

        char ch;
        cin >> ch;

        if(ch == '"') {
            cin >> skipws >> ch;

            tok.clear();

            while(ch != '"') {
                tok += ch;
                cin >> ch;
            }
            
            cout << "lis $27\n";
            cout << ".word 0xffff000c\n";

            for(auto c : tok) {
                cout << "lis $28\n";
                cout << ".word " << static_cast<int>(c) << "\n";
                cout << "sw $28, 0($27)\n";
            }

            cout << "lis $28\n";
            cout << ".word 10\n";
            cout << "sw $28, 0($27)\n";

            cin >> skipws >> tok;
        } else if(ch >= 'A' && ch <= 'Z') {
            int reg = ch - 'A' + 1;

            cin >> ch;

            if(ch == '$') {
                // Treat this as a pointer to null-terminated string 

                cout << "add $27, $" << reg << ", $0\n";

                auto startLabel = UniqueLabel();

                cout << startLabel << ":\n";

                cout << "lw $28, 0($27)\n";

                // Exit the loop if we hit the null-terminator
                cout << "beq $28, $0, 9\n";

                cout << "lis $29\n";
                cout << ".word 0xffff000c\n";
                cout << "sw $28, 0($29)\n";

                // Move the character pointer forward
                cout << "lis $28\n";
                cout << ".word 1\n";
                cout << "add $27, $27, $28\n";

                // Jump back to the start of the loop
                cout << "lis $28\n";
                cout << ".word " << startLabel << "\n";
                cout << "jr $28\n";

                cout << "lis $28\n";
                cout << ".word 10\n";
                cout << "sw $28, 0($27)\n";
            } else {
                cin.putback(ch);

                // Treat this as a number, so print the ascii representation of it with newline
                cout << "add $27, $" << reg << ", $0\n";
 
                auto startLabel = UniqueLabel();

                cout << startLabel << ":\n";

                cout << "lis $28\n";
                cout << ".word 10\n";

                cout << "div $27, $28\n";
                cout << "mflo $27\n";

                cout << "mfhi $29\n";
                cout << "lis $28\n";
                cout << ".word 48\n";
                cout << "add $28, $28, $29\n";
                cout << "lis $29\n";
                cout << ".word 0xffff000c\n";
                cout << "sw $28, 0($29)\n";

                cout << "beq $27, $0, 3\n";

                cout << "lis $28\n";
                cout << ".word " << startLabel << "\n";
                cout << "jr $28\n";

                cout << "lis $27\n";
                cout << ".word 10\n";
                cout << "lis $28\n";
                cout << ".word 0xffff000c\n";
                cout << "sw $27, 0($28)\n";    
            }

            cin >> tok;
        } else {
            cerr << "Expected string or variable.\n";
            return;
        }
    }

    void Compile(std::string& tok)
    {
        using namespace std;
        
        if(tok == "print") {
            CompilePrint(tok);
        } else if(tok == "input") {
            CompilePrint(tok);

            if(tok != ",") {
                cerr << "Expected ',' after 'input \"...\"'.\n";
                return;
            }

            string var;
            cin >> var;

            if(var.size() > 2 || var[0] < 'A' || var[0] > 'Z') {
                cerr << "Expected variable after ','.\n";
                return;
            }

            if(var.size() == 2 && var[1] != '$') {
                cerr << "Expected '$' in var name.\n";
                return;
            }

            const int maxInputSize = 32;

            // Make room for the string on the stack
            cout << "lis $27\n";
            cout << ".word " << maxInputSize << "\n";
            cout << "sub $30, $30, $27\n";
            cout << "add $27, $30, $0\n";

            int reg = var[0] - 'A' + 1;

            // Keep track of the address of the string in the variable
            cout << "add $" << reg << ", $30, $0\n";

            auto startLabel = UniqueLabel();

            cout << startLabel << ":\n";

            cout << "lis $28\n";
            cout << ".word 0xffff0004\n";

            cout << "lw $29, 0($28)\n";

            // If the character we just read is newline, then stop the loop
            cout << "lis $28\n";
            cout << ".word 10\n"; 
            cout << "beq $29, $28, 7\n";

            // Store the character into the string buffer
            cout << "sw $29, 0($27)\n";

            // Move the pointer forward
            cout << "lis $28\n";
            cout << ".word 1\n";
            cout << "add $27, $27, $28\n";

            // Hop back to the start of the loop
            cout << "lis $28\n";
            cout << ".word " << startLabel << "\n";
            cout << "jr $28\n";

            // Store null terminator
            cout << "sw $0, 0($27)\n";

            if(var.size() == 1) {
                // Interpret the string into a number
                
                // TODO(Apaar): Some kind of error checking when reading numbers
                
                const vector<int> reserved{26, 30};

                PushRegs(reserved);

                auto endLabel = UniqueLabel();

                // If the length of the string is 0, skip this shit, the resulting number is going to be 0
                cout << "add $30, $0, $0\n";
                cout << "beq $27, $" << reg << ", " << endLabel << "\n";

                cout << "lis $26\n";
                cout << ".word 1\n";

                cout << "lis $28\n";
                cout << ".word 1\n";
                cout << "sub $27, $27, $28\n";
                cout << "add $25, $0, $0\n";

                startLabel = UniqueLabel();

                cout << startLabel << ":\n";

                // We work backwards from behind the null-terminator
                cout << "lis $28\n";
                cout << ".word 45\n";   // minus (-)
                cout << "lw $29, 0($27)\n";
                
                auto skipMinusLabel = UniqueLabel();

                cout << "bne $29, $28, " << skipMinusLabel << "\n";

                // Multiply our number by -1 and exit the loop
                cout << "sub $25, $0, $25\n";
                cout << "lis $28\n";
                cout << ".word " << endLabel << "\n";
                cout << "jr $28\n";

                cout << skipMinusLabel << ":\n";
            
                cout << "lis $28\n";
                cout << ".word 48\n";
                cout << "sub $29, $29, $28\n";
                cout << "mult $29, $26\n";
                cout << "mflo $29\n";
                cout << "add $30, $30, $29\n";
                cout << "lis $28\n";
                cout << ".word 10\n";
                cout << "mult $26, $28\n";
                cout << "mflo $26\n";
                cout << "lis $28\n";
                cout << ".word " << startLabel << "\n";
                cout << "jr $28\n";

                cout << endLabel << ":\n";

                cout << "add $" << reg << ", $30, $0\n";
                PopRegs(reserved);
            }
            
            cin >> tok;
        } else if(tok == "do") {
            cin >> tok;

            auto startLabel = UniqueLabel();

            cout << startLabel << ":\n";

            while(tok != "loop") {
                Compile(tok);

                if(!cin) {
                    cerr << "Expected 'loop' to close off 'do'.\n";
                }
            }

            cout << "lis $27\n";
            cout << ".word " << startLabel << "\n";
            cout << "jr $27\n";

            cin >> tok;
        } else if(tok.size() == 1 && tok[0] >= 'A' && tok[0] <= 'Z') {
            int reg = tok[0] - 'A' + 1;

            cin >> tok;

            if(tok != "=") {
                cerr << "Expected '=' after '" << tok << "'\n";
                return;
            }

            cin >> tok;

            CompileArith(tok, 27);

            cout << "add $" << reg << ", $27, $0\n";     
        } else {
            cerr << "Unexpected token '" << tok << "'\n";
            cin >> tok;
        }
    }
}

int main(int argc, char** argv)
{
    using namespace std;

    string tok;
    cin >> tok;

    while(cin) {
        Compile(tok);
    }

    cout << "jr $31\n";
    return 0;
}
