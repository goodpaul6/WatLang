#include <sstream>
#include <vector>
#include <unordered_map>

struct Codegen
{
    // Assembles str into instructions
    void parse(Pos pos, const std::string& str)
    {
        std::istringstream s{str};

        std::string temp;

        s >> temp;

        if(temp == ".word") {
            s >> temp;
            
            if(isdigit(temp[0])) {
                long long value;
                try {
                    value = std::stoll(temp, nullptr, 0);
                } catch(...) {
                    throw PosError{pos, "Failed to convert to value: " + temp};
                }

                if(value < -2147483647L && value > 4294967295) {
                    throw PosError{pos, "Word value out of range: " + std::to_string(value)};
                }

                word(static_cast<int32_t>(value));
            } else if(isalpha(temp[0])) {
                patches.emplace_back(Patch::WORD, code.size(), temp);
                word(0);
            }
        } else if(temp[temp.size() - 1] == ':') {
            labelHere(temp.substr(0, temp.size() - 1));
        } else if(temp == "lis") {
            s >> temp;
            lis(parseReg(pos, temp));
        } else if(temp == "add" || temp == "sub" || temp == "slt") {
            auto instr = temp;

            int regs[3];

            for(int i = 0; i < 3; ++i) {
                if(!(s >> temp)) {
                    throw PosError{pos, "Expected register"};
                }

                regs[i] = parseReg(pos, temp);
            }

            if(instr == "add") add(regs[0], regs[1], regs[2]);
            else if(instr == "sub") sub(regs[0], regs[1], regs[2]);
            else if(instr == "slt") slt(regs[0], regs[1], regs[2]);
        } else if(temp == "mult" || temp == "div") {
            auto instr = temp;
            
            int regs[2];

            for(int i = 0; i < 2; ++i) {
                if(!(s >> temp)) {
                    throw PosError{pos, "Expected register"};
                }

                regs[i] = parseReg(pos, temp);
            }

            if(instr == "mult") mult(regs[0], regs[1]);
            else if(instr == "div") div(regs[0], regs[1]);
        } else if(temp == "beq" || temp == "bne") {
            auto instr = temp;

            int regs[2];

            for(int i = 0; i < 2; ++i) {
                if(!(s >> temp)) {
                    throw PosError{pos, "Expected register"};
                }

                regs[i] = parseReg(pos, temp);
            }
            
            s >> temp;

            int16_t imm = 0;
        
            if(isalpha(temp[0])) {
                patches.emplace_back(Patch::BRANCH, code.size(), temp);
            } else {
                auto off = std::stoi(temp, nullptr, 0);

                if(off < -32768 && off > 32767) {
                    throw PosError{pos, "Branch offset out of range"};
                }

                imm = static_cast<int16_t>(off);
            }

            if(instr == "beq") beq(regs[0], regs[1], imm);
            else if(instr == "bne") bne(regs[0], regs[1], imm);
        } else if(temp == "lw" || temp == "sw") {
            auto instr = temp;

            s >> temp;

            int t = parseReg(pos, temp); 

            int off;

            s >> off;

            if(off < -32768 && off > 32767) {
                throw PosError{pos, "Memory offset out of range"};
            }

            auto imm = static_cast<int16_t>(off);

            char c = 0;

            s >> c;

            if(c != '(') {
                throw PosError{pos, "Expected '(' after offset"};
            }

            temp.clear();

            while(s >> c && c != ')') {
                temp += c;
            }

            s >> c;

            int sr = parseReg(pos, temp);

            if(instr == "lw") lw(t, imm, sr);
            else if(instr == "sw") sw(t, imm, sr);
        } else if(temp == "jr" || temp == "jalr" || temp == "mfhi" || temp == "mflo") {
            auto instr = temp;

            s >> temp;

            if(instr == "jr") jr(parseReg(pos, temp));
            else if(instr == "jalr") jalr(parseReg(pos, temp));
            else if(instr == "mfhi") mfhi(parseReg(pos, temp));
            else if(instr == "mflo") mflo(parseReg(pos, temp));
        } else {
            throw PosError{pos, "Expected instruction but got " + temp};
        }
    }

    // Get the position in memory of the next instruction
    int32_t getPos() const
    {
        return static_cast<int32_t>(code.size() * sizeof(Instruction));
    }

    void labelHere(std::string name)
    {
        if(labels.find(name) != labels.end()) {
            throw std::runtime_error{"Defined multiple labels with the name " + name};
        }

        labels[std::move(name)] = static_cast<int32_t>(code.size());
    }
    
    void lis(int reg)
    {
        code.emplace_back(rInst(Instruction::LIS, 0, 0, reg));
    }

    void word(int32_t value)
    {
        code.emplace_back(wInst(value));
    }
    
    void word(const std::string& labelName)
    {
        // This will be patched
        patches.emplace_back(Patch::WORD, code.size(), labelName);
        code.emplace_back(wInst(0));
    }

    void lis(int reg, int32_t value)
    {
        lis(reg);
        word(value);
    }

    void lis(int reg, const std::string& labelName)
    {
        lis(reg);
        word(labelName);
    }

    void add(int d, int s, int t)
    {
        code.emplace_back(rInst(Instruction::ADD, s, t, d));
    }

    void sub(int d, int s, int t)
    {
        code.emplace_back(rInst(Instruction::SUB, s, t, d));
    }

    void mult(int s, int t)
    {
        code.emplace_back(rInst(Instruction::MULT, s, t, 0));
    }

    void div(int s, int t)
    {
        code.emplace_back(rInst(Instruction::DIV, s, t, 0));
    }

    void slt(int d, int s, int t)
    {
        code.emplace_back(rInst(Instruction::SLT, s, t, d));
    }

    void mfhi(int d)
    {
        code.emplace_back(rInst(Instruction::MFHI, 0, 0, d));
    }

    void mflo(int d)
    {
        code.emplace_back(rInst(Instruction::MFLO, 0, 0, d));
    }

    void lw(int t, int16_t imm, int s)
    {
        code.emplace_back(iInst(Instruction::LW, s, t, imm));
    }

    void sw(int t, int16_t imm, int s)
    {
        code.emplace_back(iInst(Instruction::SW, s, t, imm));
    }

    void beq(int s, int t, int16_t imm)
    {
        code.emplace_back(iInst(Instruction::BEQ, s, t, imm));
    }

    void beq(int s, int t, const std::string& labelName)
    {
        patches.emplace_back(Patch::BRANCH, code.size(), labelName);
        code.emplace_back(iInst(Instruction::BEQ, s, t, 0));
    }

    void bne(int s, int t, int16_t imm)
    {
        code.emplace_back(iInst(Instruction::BNE, s, t, imm));
    }

    void bne(int s, int t, const std::string& labelName)
    {
        patches.emplace_back(Patch::BRANCH, code.size(), labelName);
        code.emplace_back(iInst(Instruction::BNE, s, t, 0));
    }

    void jr(int s)
    {
        code.emplace_back(rInst(Instruction::JR, s, 0, 0));
    }

    void jalr(int s)
    {
        code.emplace_back(rInst(Instruction::JALR, s, 0, 0));
    }

    std::vector<Instruction> getPatchedCode() const
    {
        // This just patches all the labels with the correct address
        auto result = code;

        for(auto& patch : patches) {
            auto found = labels.find(patch.getLabelName());

            if(found == labels.end()) {
                throw std::runtime_error{"Referenced undefined label " + patch.getLabelName()};
            }

            switch(patch.getType()) {
                case Patch::WORD: {
                    result[patch.getPos()].word = found->second * sizeof(Instruction);
                } break;

                case Patch::BRANCH: {
					int32_t isize = sizeof(Instruction);
                    int32_t off = (found->second * isize - static_cast<int32_t>(patch.getPos()) * isize - isize) / isize;
                    
                    if(off < -32768 && off > 32767) {
                        throw std::runtime_error{"Branch to label " + found->first + " is out of branch offset range (" + std::to_string(off) + ")"};
                    }
                    
					result[patch.getPos()].word |= static_cast<int16_t>(off) & 0xffff;
                } break;
            }
        }

        return result;
    }

private:
    struct Patch
    {
        enum Type
        {
            WORD,
            BRANCH
        };

        Patch(Type type, size_t pos, std::string labelName) : type{type}, pos{pos}, labelName{std::move(labelName)} {}

        Type getType() const { return type; }
        size_t getPos() const { return pos; }
        const std::string& getLabelName() const { return labelName; }
    
    private:
        Type type;
        size_t pos;
        std::string labelName;
    };
    
    std::unordered_map<std::string, int> labels;
    std::vector<Instruction> code;
    std::vector<Patch> patches;

    Instruction wInst(int32_t value)
    {
        Instruction i;
        i.word = value;
    
        return i;
    }

    Instruction iInst(Instruction::Type type, int s, int t, int16_t imm)
    {
        Instruction i;
        i.word = ((type & 0xf) << 28) | ((s & 0x1f) << 23) | ((t & 0x1f) << 18) | (imm & 0xffff);

        return i;
    }

    Instruction rInst(Instruction::Type type, int s, int t, int d)
    {
        Instruction i;
        i.word = ((type & 0xf) << 28) | ((s & 0x1f) << 23) | ((t & 0x1f) << 18) | ((d & 0x1f) << 13);

        return i;
    }

    int parseReg(const Pos& pos, const std::string& str)
    {
        if(str[0] != '$') {
            throw PosError{pos, "Expected '$' in register operand"};
        }

        auto reg = std::stoi(str.substr(1));
        
        if(reg < 0 || reg > 31) {
            throw PosError{pos, "Invalid register: " + str};
        }

        return reg;
    }
};
