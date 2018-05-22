#include "../WatLang/basic.wat"

func traverse(ap, index) {
    if(index < 0) {
        return;
    }

    var np = ap + index * 4

    putn(*np)

    traverse(ap, *(np + 4))
    traverse(ap, *(np + 8))
}

func main() {
    var ap = 0

    asm "lis $3"
    asm ".word start"
    asm "jr $3"
    
    asm "tree:"
    asm ".word 10"
    asm ".word -1"
    asm ".word 3"
    asm ".word 5"
    asm ".word -1"
    asm ".word 6"
    asm ".word 2"
    asm ".word -1"
    asm ".word -1"

    asm "start:"

    asm "lis $1"
    asm ".word tree"

    traverse(ap, 0)
}
