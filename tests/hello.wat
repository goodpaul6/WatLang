func putc(c : char) : void {
    *cast(*char) 0xffff000c = c;
}

func main() : void {
    putc('H');
    putc('e');
    putc('l');
    putc('l');
    putc('o');
    putc(cast(char) 10);
}
