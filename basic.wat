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
    *cast(*char) 0xffff000c = c;
}

func puts(s : *char) : void {
    // Load the char pointer
    asm "lw $1 0($29)";

    asm "lis $2";
    asm ".word 0xffff000c";

    asm "lis $3";
    asm ".word 4";

    asm "putsLoop:";
    asm "lw $4 0($1)";
    asm "beq $4 $0 putsEnd";
    asm "sw $4 0($2)";
    asm "add $1 $1 $3";
    asm "lis $4";
    asm ".word putsLoop";
    asm "jr $4";

    asm "putsEnd:";
    putc(cast(char) 10);
}

func putn(n : int) : void {
    if(n == 0) {
        putc('0');
        putc(cast(char) 10);
        return;
    }

    if(n < 0) {
        *cast(*char) 0xffff000c = '-';
        n = n * -1;
    }

    var buf : *char = [30]"";
    var bufStart : *char = buf;

    while(n > 0) {
        *buf = '0' + cast(char)(n % 10);
        buf = buf + 4;
        n = n / 10;
    }

    while(buf != bufStart) {
        *cast(*char) 0xffff000c = *(buf - 4);
        buf = buf - 4;
    }

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
