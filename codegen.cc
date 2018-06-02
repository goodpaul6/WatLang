#include <vector>

struct Instruction
{
    enum Type
    {
        LIS, WORD,
        ADD, SUB, MULT, DIV, SLT,
        MFHI, MFLO,
        LW, SW,
        BEQ, BNE,
        JR, JALR
    };

    Type type;

    union
    {
        int32_t word;
        struct 
        { 
            int s, t;
            union { int d; int16_t imm; };
        };
    };
};

struct Codegen
{
    void labelHere(std::string name)
    {
        for(auto& label : labels) {
            if(label.first == name) {
                if(label.second < 0) {
                    // Fill in the position
                    label.second = static_cast<int32_t>(code.size());
                    return;
                } else {
                    throw std::runtime_error{"Defined multiple labels with the name " + name};
                }
            }
        }

        labels.emplace_back(std::move(name), static_cast<int32_t>(code.size()));
    }

    void loadConst(int reg, int32_t value)
    {
        code.emplace_back(rInst(Instruction::LIS, 0, 0, reg));
        code.emplace_back(word(value));
    }

    void loadLabel(int reg, const std::string& labelName)
    {
        code.emplace_back(rInst(Instruction::LIS, 0, 0, reg));
        code.emplace_back(word(getLabel(labelName)));
    }

    void add(int s, int t, int d)
    {
        code.emplace_back(rInst(Instruction::ADD, s, t, d));
    }

    void sub(int s, int t, int d)
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

    void slt(int s, int t, int d)
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

    void lw(int s, int t, int16_t imm)
    {
        code.emplace_back(iInst(Instruction::LW, s, t, imm));
    }

    void sw(int s, int t, int16_t imm)
    {
        code.emplace_back(iInst(Instruction::SW, s, t, imm));
    }

    void jr(int s)
    {
        code.emplace_back(rInst(Instruction::JR, s, 0, 0));
    }

    void jalr(int s)
    {
        code.emplace_back(rInst(Instruction::JALR, s, 0, 0));
    }

private:
    using Label = std::pair<std::string, int32_t>;

    std::vector<Label> labels;
    std::vector<Instruction> code;

    int32_t getLabel(std::string name)
    {
        auto i = 0u;

        for(auto& label : labels) {
            if(label.first == name) {
                return i;
            }
            ++i;
        }

        // We create a label with an undefined position
        // which will be filled in later
        labels.emplace_back(std::move(name), -1);
        return labels.size() - 1;
    }

    Instruction word(int32_t value)
    {
        Instruction i;

        i.type = Instruction::WORD;
        i.word = value;
    
        return i;
    }

    Instruction iInst(Instruction::Type type, int s, int t, int16_t imm)
    {
        Instruction i;

        i.type = type;
        i.s = s;
        i.t = t;
        i.imm = imm;

        return i;
    }

    Instruction rInst(Instruction::Type type, int s, int t, int d)
    {
        Instruction i;

        i.type = type;
        i.s = s;
        i.t = t;
        i.d = d;

        return i;
    }
};
