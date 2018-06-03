#include <string>
#include <cassert>
#include <iostream>

struct Instruction
{
    enum Type
    {
        LIS,
        ADD, SUB, MULT, DIV, SLT,
        MFHI, MFLO,
        LW, SW,
        BEQ, BNE,
        JR, JALR
    };

    // rFormat:
    // oooo ssss sttt ttdd ddd0 0000 0000 0000
    // iFormat:
    // oooo ssss sttt tt00 iiii iiii iiii iiii
    int32_t word;

    Type getType() const
    {
        return static_cast<Type>((word >> 28) & 0xf);
    }

    uint8_t getS() const { return (word >> 23) & 0x1f; }
    uint8_t getT() const { return (word >> 18) & 0x1f; }
    uint8_t getD() const { return (word >> 13) & 0x1f; }

    int16_t getImm() const { return word & 0xffff; }
};

void run(const Instruction* code, size_t codeSize)
{    
    const size_t isize = sizeof(Instruction);

    const int32_t exitAddress = -1;
    const uint32_t getcAddress = 0xffff0004;
    const uint32_t putcAddress = 0xffff000c;

    int32_t lo = 0, hi = 0;
    int32_t pc = 0;
    int32_t regs[32] = { 0 };
    
    static uint8_t mem[1 << 16];

    memcpy(mem, code, codeSize);

    // Initialize the special registers
    regs[30] = 1 << 16;
    regs[31] = exitAddress;
 
    while(true) {
        if(pc == exitAddress) {
            return;
        }

        auto instr = *reinterpret_cast<Instruction*>(&mem[pc]);
        
        auto s = instr.getS();
        auto t = instr.getT();
        auto d = instr.getD();

        int16_t imm = instr.getImm();

        regs[0] = 0;

        switch(instr.getType()) {
			default: {
				throw std::runtime_error{ "Invalid instruction type: " + std::to_string(instr.getType()) };
			} break;
			
            case Instruction::LIS: {
                pc += isize;
				regs[d] = *reinterpret_cast<int32_t*>(&mem[pc]);
                pc += isize;
            } break;

            case Instruction::ADD: {
                regs[d] = regs[s] + regs[t];
                pc += isize;
            } break;

            case Instruction::SUB: {
                regs[d] = regs[s] - regs[t];
                pc += isize;
            } break;

            case Instruction::MULT: {
                int64_t result = static_cast<int64_t>(regs[s]) * regs[t];
                lo = static_cast<int32_t>(result);
                hi = static_cast<int32_t>(result >> 32);
                pc += isize;
            } break;

            case Instruction::DIV: {
                lo = regs[s] / regs[t];
                hi = regs[s] % regs[t];
                pc += isize;
            } break;

            case Instruction::SLT: {
                regs[d] = regs[s] < regs[t];
                pc += isize;
            } break;

            case Instruction::MFHI: {
                regs[d] = hi;
                pc += isize;
            } break;

            case Instruction::MFLO: {
                regs[d] = lo;
                pc += isize;
            } break;

            case Instruction::LW: {
                uint32_t addr = static_cast<uint32_t>(regs[s]);
                addr += imm;
                
                if(addr == getcAddress) {
                    regs[t] = getchar();
                } else {
                    regs[t] = *reinterpret_cast<int32_t*>(&mem[addr]);
                }

                pc += isize;
            } break;

            case Instruction::SW: {
                uint32_t addr = static_cast<uint32_t>(regs[s]);
                addr += imm;
                
                if(addr == putcAddress) {
                    putchar(regs[t]);
                } else {
                    *reinterpret_cast<int32_t*>(&mem[addr]) = regs[t];
                }

                pc += isize;
            } break;

            case Instruction::BEQ: {
                pc += isize;
                if(regs[s] == regs[t]) {
                    pc += imm * isize;
                }
            } break;

            case Instruction::BNE: {
                pc += isize;
                if(regs[s] != regs[t]) {
                    pc += imm * isize;
                }
            } break;

            case Instruction::JR: {
                pc = regs[s];
            } break;

            case Instruction::JALR: {
				pc += isize;
                int32_t temp = regs[s];
                regs[31] = pc;
                pc = temp;
            } break;
        }
    }
}
