#include <assert.h>
#include <vector>
#include <iostream>
#include <string>

// Reserved registers
// $1-$26 - Variables A to Z
// $27-29 - Temporary regs

namespace
{ 
    int PC = 0;

    void PushRegs(const std::vector<int>& regs)
    {
        using namespace std;

        assert(regs.size() > 0);

        int off = 0;

        for(auto reg : regs) {
            off -= 4;
            cout << "sw $" << reg << ", " << off << "($30)\n";
            ++PC;
        }

        cout << "lis $" << regs[0] << "\n";
        cout << ".word " << -off << "\n";
        cout << "sub $30, $30, $" << regs[0] << "\n";

        PC += 3;
    }

    void PopRegs(const std::vector<int>& regs)
    {
        using namespace std;

        assert(regs.size() > 0);

        int off = regs.size() * -4;

        cout << "lis $" << regs[0] << "\n";
        cout << ".word " << -off << "\n";
        cout << "add $30, $30, $" << regs[0] << "\n";

        PC += 3;

        for(auto it = regs.rbegin(); it != regs.rend(); ++it) {
            cout << "lw $" << *it << ", " << off << "($30)\n";
            off += 4;
            ++PC;
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

            PC += 2;

            for(auto c : tok) {
                cout << "lis $28\n";
                cout << ".word " << static_cast<int>(c) << "\n";
                cout << "sw $28, 0($27)\n";
                PC += 3;
            }

            cin >> skipws >> tok;
        } else if(ch >= 'A' && ch <= 'Z') {
            // If it's a variable, we treat it as the address of the start of a null-terminated string in memory.
            
            cout << "add $27, $" << (ch - 'A' + 1) << ", $0\n";

            PC += 1;

            int startPC = PC;

            cout << "lw $28, 0($27)\n";
            
            // Exit the loop if we hit the null-terminator
            cout << "beq $28, $0, 9";

            cout << "lis $29\n";
            cout << ".word 0xffff000c\n";
            cout << "sw $28, 0($29)\n";
            
            // Move the character pointer forward
            cout << "lis $28\n";
            cout << ".word 1\n";
            cout << "add $27, $27, $28\n";

            // Jump back to the start of the loop
            cout << "lis $28\n";
            cout << ".word " << startPC << "\n";
            cout << "jr $28\n";

            PC += 11;

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

            if(var.size() > 1 || var[0] < 'A' || var[0] > 'Z') {
                cerr << "Expected variable after ','.\n";
                return;
            }

            const int maxInputSize = 32;

            // Make room for the string on the stack
            cout << "lis $27\n";
            cout << ".word " << maxInputSize << "\n";
            cout << "sub $30, $30, $27\n";

            // Keep track of the address of the start of the string buffer (which is just the current stack address)
            // in the variable and in register 27
            cout << "add $" << (var[0] - 'A' + 1) << ", $30, $0\n";
            cout << "add $27, $30, $0\n";

            PC += 5;

            int startPC = PC;

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
            cout << ".word " << startPC << "\n";
            cout << "jr $28\n";

            // Store null terminator
            cout << "sw $0, 0($27)\n";

            PC += 14;
            
            cin >> tok;
        } else if(tok == "do") {
            cin >> tok;

            int startPC = PC;

            while(tok != "loop") {
                Compile(tok);

                if(!cin) {
                    cerr << "Expected 'loop' to close off 'do'.\n";
                }
            }

            cout << "lis $27\n";
            cout << ".word " << startPC << "\n";
            cout << "jr $27\n";

            PC += 3;

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
