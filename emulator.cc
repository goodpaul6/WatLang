#include <cassert>
#include <iostream>

void runMips(const uint8_t* prog, size_t progSize)
{
    const int32_t exitAddress = 0x1234beef;
    const int32_t getcAddress = 0xffff0004;
    const int32_t putcAddress = 0xffff000c;

    int32_t lo = 0, hi = 0;
    uint32_t pc = 0;
    int32_t regs[32] = { 0 };
    
    uint8_t mem[1 << 20];

    assert(progSize < 1 << 20);

    memcpy(mem, prog, progSize);

    // Initialize the special registers
    regs[30] = 1 << 20;
    regs[31] = exitAddress;
 
    while(true) {
        uint32_t instr = static_cast<uint32_t>(mem[pc]);

        auto startBits = (instr >> 26) & 0x3f;

        auto sReg = (instr >> 21) & 0x3f;
        auto tReg = (instr >> 16) & 0x3f;
        auto dReg = (instr >> 11) & 0x3f;

        auto endBits = instr & 0x7ff;

        auto imm = instr & 0xffff;

        if(startBits == 0) {
            // add, sub, mult, div, mfhi, lis, slt, jr, jalr

            switch(endBits) {
                case 0x20: {
                    // add
                    regs[dReg] = regs[sReg] + regs[tReg];
                    pc += 4;
                } break;

                case 0x22: {
                    // sub
                    regs[dReg] = regs[sReg] - regs[tReg];
                    pc += 4;
                } break;

                case 0x18: {
                    // mult
                    int64_t r = static_cast<int64_t>(regs[sReg]) * static_cast<int64_t>(regs[tReg]);

                    hi = static_cast<int32_t>(r >> 32);
                    lo = static_cast<int32_t>(r);

                    pc += 4;
                } break;

                case 0x1a: {
                    // div
                    lo = regs[sReg] / regs[tReg];
                    hi = regs[sReg] % regs[tReg];

                    pc += 4;
                } break;

                case 0x10: {
                    // mfhi
                    regs[dReg] = hi;

                    pc += 4;
                } break;

                case 0x12: {
                    // mflo
                    regs[dReg] = lo;

                    pc += 4;
                } break;

                case 0x14: {
                    // lis
                    pc += 4;

                    regs[dReg] = *static_cast<int32_t*>(&mem[pc]);

                    pc += 4;
                } break;

                case 0x2A: {
                    // slt
                    regs[dReg] = regs[sReg] < regs[tReg];
                    
                    pc += 4;
                } break;

                case 0x08: {
                    // jr
                    if(regs[sReg] == exitAddress) {
                        // Done!
                        return;
                    }

                    pc = regs[sReg];
                } break;

                case 0x09: {
                    int32_t temp = regs[sReg];
                    regs[31] = pc;
                    pc = temp;
                } break;
            }
        } else {
            // lw, sw, beq

            switch(startBits) {
                case 0x23: {
                    // lw
                    int32_t addr = imm + regs[sReg];

                    // TODO(Apaar): Check for unaligned reads

                    if(addr == getcAddress) {
                        regs[tReg] = getchar();
                    } else {
                        regs[tReg] = *static_cast<int32_t*>(&mem[addr]);
                    }
                } break;

                case 0x2B: {
                    // sw
                    int32_t addr = imm + regs[sReg];

                    // TODO(Apaar): Check for unaligned writes

                    if(addr == putcAddress) {
                        putchar(regs[tReg]);
                    } else {
                        *static_cast<int32_t*>(&mem[addr]) = regs[tReg];
                    }
                } break;
            }
        }
    }
}
