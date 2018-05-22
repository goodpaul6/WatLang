#include "basic.wat"

func main() {
    var a = 0

    asm "lis $3"
    asm ".word start"
    asm "jr $3"
    
    asm "array:"
    asm ".word 10"
    asm ".word 20"
    asm ".word 30"
    asm ".word 40"
    asm ".word 50"

    asm "start:"
    
    asm "lis $1"
    asm ".word array"

    var i = 0
    while(i < 5) {
        putn(*a)
        a = a + 4
        i = i - -1
    }
}
