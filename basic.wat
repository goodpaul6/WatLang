func putc(c) {
    asm "lis $3"
    asm ".word 0xffff000c"
    asm "sw $1, 0($3)"
}

func puts(s) {
    asm "lis $3"
    asm ".word 0xffff000c"

    asm "lis $5"
    asm ".word 4"

    asm "putsLoop:"
    asm "lw $4, 0($1)"
    asm "beq $4, $0, putsEnd"
    asm "sw $4, 0($3)"
    asm "add $1, $1, $5"
    asm "lis $4"
    asm ".word putsLoop"
    asm "jr $4"

    asm "putsEnd:"
    putc(10)
}

func putn(n) {
    // If the number is 0, we just output that and exit
    asm "bne $1, $0, putnNonZero"
    asm "lis $3"
    asm ".word 48"
    asm "lis $4"
    asm ".word 0xffff000c"
    asm "sw $3, 0($4)"
    asm "lis $3"
    asm ".word putnExit"
    asm "jr $3"

    asm "putnNonZero:"

    // If the number is < 0, print the sign and negate it
    asm "slt $3, $1, $0"
    asm "beq $3, $0, putnPostSign"
    asm "lis $3"
    asm ".word 45"
    asm "lis $4"
    asm ".word 0xffff000c"
    asm "sw $3, 0($4)"
    asm "sub $1, $0, $1"

    asm "putnPostSign:"

    // Make room on the stack for 20 digits (4 * 20 = 80 bytes)
    asm "lis $3"
    asm ".word 80"
    asm "sub $30, $30, $3"
    
    // We store the digit pointer in $4
    asm "add $4, $30, $0"

    // $5 is 10 for the first loop
    asm "lis $5"
    asm ".word 10"

    asm "putnstart:"
    asm "beq $1, $0, putnprint"
    asm "div $1, $5"

    // Put the result of the modulo into digit memory and move the pointer
    asm "mfhi $6"
    asm "mflo $1"
    asm "sw $6, 0($4)"
    asm "lis $6"

    asm ".word 4"
    asm "add $4, $4, $6"

    asm "lis $6"
    asm ".word putnstart"
    asm "jr $6"
    
    asm "putnprint:"

    asm "lis $5"
    asm ".word 0xffff000c"

    asm "putnprintloop:"

    asm "beq $4, $30, putnEnd"
    asm "lw $6, -4($4)"
    asm "lis $7"
    asm ".word 48"
    asm "add $6, $6, $7"
    asm "sw $6, 0($5)"

    asm "lis $6"
    asm ".word 4"
    asm "sub $4, $4, $6"

    asm "lis $6"
    asm ".word putnprintloop"
    asm "jr $6"

    asm "putnEnd:"

    asm "lis $3"
    asm ".word 80"
    asm "add $30, $30, $3"

    asm "putnExit:"

    putc(10)
}
