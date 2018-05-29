#include "basic.wat"

func fact(n : int) : int {
    if(n < 2) return 1;
    return n * fact(n - 1);
}

func main(a : int, b : int) : void {
    putn(fact(a));
}
