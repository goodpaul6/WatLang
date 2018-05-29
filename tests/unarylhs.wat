func putc(c : char) : void {
    *cast(*char) 0xffff000c = c;
}

func puts(s : *char) : void {
    while (*s != cast(char) 0) {
        putc(*s);
        s = s + 4;
    }
}

func main() : void {
    puts("Hello");
    putc(cast(char) 10);
}
