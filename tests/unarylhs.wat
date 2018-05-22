func putc(c) {
    *0xffff000c = c
}

func main() {
    putc('H')
    putc('e')
    putc('l')
    putc('l')
    putc('o')
    putc(10)
}
