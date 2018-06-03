func exit() : void {
    asm "lis $1";
    asm ".word exitAddrGlobalXXXX";
    asm "lw $31 0($1)";
    asm "jr $31";
}

func assert(x : bool, message : *char, filename: *char, line: int) : void {
    if(x == false) {
        puts("ASSERTION FAILED:");

        puts("File:");
        puts(filename);

        puts("Line:");
        putn(line);

        puts("Message:");
        puts(message);
        exit();
    }
}

func getc() : char {
    return *cast(*char) 0xffff0004;
}

func putc(c : char) : void {
    asm "lis $3";
    asm ".word 0xffff000c";
    asm "sw $1 0($3)";
}

func puts(s : *char) : void {
    asm "lis $3";
    asm ".word 0xffff000c";

    asm "lis $5";
    asm ".word 4";

    asm "putsLoop:";
    asm "lw $4 0($1)";
    asm "beq $4 $0 putsEnd";
    asm "sw $4 0($3)";
    asm "add $1 $1 $5";
    asm "lis $4";
    asm ".word putsLoop";
    asm "jr $4";

    asm "putsEnd:";
    putc(cast(char) 10);
}

func putn(n : int) : void {
    // If the number is 0, we just output that and exit
    asm "bne $1 $0 putnNonZero";
    asm "lis $3";
    asm ".word 48";
    asm "lis $4";
    asm ".word 0xffff000c";
    asm "sw $3 0($4)";
    asm "lis $3";
    asm ".word putnExit";
    asm "jr $3";

    asm "putnNonZero:";

    // If the number is < 0, print the sign and negate it
    asm "slt $3 $1 $0";
    asm "beq $3 $0 putnPostSign";
    asm "lis $3";
    asm ".word 45";
    asm "lis $4";
    asm ".word 0xffff000c";
    asm "sw $3 0($4)";
    asm "sub $1 $0 $1";

    asm "putnPostSign:";

    // Make room on the stack for 20 digits (4 * 20 = 80 bytes)
    asm "lis $3";
    asm ".word 80";
    asm "sub $30 $30 $3";
    
    // We store the digit pointer in $4
    asm "add $4 $30 $0";

    // $5 is 10 for the first loop
    asm "lis $5";
    asm ".word 10";

    asm "putnstart:";
    asm "beq $1 $0 putnprint";
    asm "div $1 $5";

    // Put the result of the modulo into digit memory and move the pointer
    asm "mfhi $6";
    asm "mflo $1";
    asm "sw $6 0($4)";
    asm "lis $6";

    asm ".word 4";
    asm "add $4 $4 $6";

    asm "lis $6";
    asm ".word putnstart";
    asm "jr $6";
    
    asm "putnprint:";

    asm "lis $5";
    asm ".word 0xffff000c";

    asm "putnprintloop:";

    asm "beq $4 $30 putnEnd";
    asm "lw $6 -4($4)";
    asm "lis $7";
    asm ".word 48";
    asm "add $6 $6 $7";
    asm "sw $6 0($5)";

    asm "lis $6";
    asm ".word 4";
    asm "sub $4 $4 $6";

    asm "lis $6";
    asm ".word putnprintloop";
    asm "jr $6";

    asm "putnEnd:";

    asm "lis $3";
    asm ".word 80";
    asm "add $30 $30 $3";

    asm "putnExit:";

    putc(cast(char) 10);
}

func strcmp(a : *char, b : *char) : int {
    while(*a != 0 && *b != 0) {
        if(*a != *b) {
            return cast(int) (*a - *b);
        }

        a = a + 4;
        b = b + 4;
    }

    return cast(int) (*a - *b);
}

func strcpy(dest : *char, src : *char) : void {
    while(*src != cast(char) 0) {
        *dest = *src;
        dest = dest + 4;
        src = src + 4;
    }

    *dest = cast(char) 0;
}

func strcat(dest : *char, src : *char) : void {
    var end : *char = dest;

    while(*end != 0) {
        end = end + 4;
    }

    while(*src != 0) {
        *end = *src;
        end = end + 4;
        src = src + 4;
    }

    *end = cast(char) 0;
}

var allocPos : *void;

func initAlloc() : void {
    var start : *void = cast(*void) 0;

    asm "lis $2";
    asm ".word memStartXXXX";

    allocPos = start;
}

func malloc(size : int) : *void {
    var mem : *void = allocPos;
    
    // Prefix with size
    var pSize : *int = cast(*int) allocPos;
    
    *pSize = 4 + size;
    allocPos = allocPos + 4 + size;

    return mem;
}

func free(mem : *void) : void {
    allocPos = allocPos - (*cast(*int) (mem - 4));
}
