#include "basic.wat"

func fact(n) {
    if(n < 2) return 1
    return n * fact(n - 1)
}

func main() {
    putn(fact(5))
}
