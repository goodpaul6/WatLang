func putc(c) {
    asm "lis $3"
    asm ".word 0xffff000c"
    asm "sw $1, 0($3)"
}

func main() {
    putc(48)
    putc(10)
}
