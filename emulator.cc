#include <cassert>
#include <iostream>

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
            uint8_t s, t;
            union { uint8_t d; int16_t imm; };
        };
    };
};

void run(const Instruction* code, size_t codeSize)
{
    const size_t isize = sizeof(Instruction);

    const int32_t exitAddress = -1;
    const int32_t getcAddress = 0xffff0004;
    const int32_t putcAddress = 0xffff000c;

    int32_t lo = 0, hi = 0;
    int32_t pc = 0;
    int32_t regs[32] = { 0 };
    
    uint8_t mem[1 << 20];

    memcpy(mem, code, codeSize);

    // Initialize the special registers
    regs[30] = 1 << 20;
    regs[31] = exitAddress;
 
    while(true) {
        if(pc == exitAddress) {
            return;
        }

        auto instr = *static_cast<Instruction*>(&mem[pc]);

        regs[0] = 0;

        switch(instr.type) {
            case Instruction::LIS: {
                pc += isize;
                regs[instr.d] = static_cast<Instruction*>(&mem[pc])->word;
                pc += isize;
            } break;

            case Instruction::ADD: {
                regs[instr.d] = regs[instr.s] + regs[instr.t];
                pc += isize;
            } break;

            case Instruction::SUB: {
                regs[instr.d] = regs[instr.s] - regs[instr.t];
                pc += isize;
            } break;

            case Instruction::MULT: {
                int64_t result = static_cast<int64_t>(regs[instr.s]) * regs[instr.t];
                lo = static_cast<int32_t>(result);
                hi = static_cast<int32_t>(result >> 32);
                pc += isize;
            } break;

            case Instruction::DIV: {
                lo = regs[instr.s] / regs[instr.t];
                hi = regs[instr.s] % regs[instr.t];
                pc += isize;
            } break;

            case Instruction::SLT: {
                regs[instr.d] = regs[instr.s] < regs[instr.t];
                pc += isize;
            } break;

            case Instruction::MFHI: {
                regs[instr.d] = hi;
                pc += isize;
            } break;

            case Instruction::MFLO: {
                regs[instr.d] = lo;
                pc += isize;
            } break;

            case Instruction::LW: {
                int32_t addr = regs[instr.s] + instr.imm;
                
                if(addr == getcAddress) {
                    regs[instr.t] = getchar();
                } else {
                    regs[instr.t] = *static_cast<int32_t*>(mem[addr]);
                }

                pc += isize;
            } break;

            case Instruction::SW: {
                int32_t addr = regs[instr.s] + instr.imm;
                
                if(addr == putcAddress) {
                    putchar(regs[instr.t]);
                } else {
                    *static_cast<int32_t*>(mem[addr]) = regs[instr.t];
                }

                pc += isize;
            } break;

            case Instruction::BEQ: {
                pc += isize;
                if(regs[instr.s] == regs[instr.t]) {
                    pc += instr.imm * isize;
                }
            } break;

            case Instruction::BNE: {
                pc += isize;
                if(regs[instr.s] != regs[instr.t]) {
                    pc += instr.imm * isize;
                }
            } break;

            case Instruction::JR: {
                pc = regs[instr.s];
            } break;

            case Instruction::JALR: {
                int32_t temp = regs[instr.s];
                regs[31] = pc;
                pc = temp;
            } break;
        }
    }
}
