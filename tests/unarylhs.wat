func putc(c) {
    *0xffff000c = c
}

func puts(s) {
    while (*s) {
        putc(*s)
        s = s + 4
    }
}

func main() {
    puts("Hello")
    putc(10)
}
