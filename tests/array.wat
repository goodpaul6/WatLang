#include "basic.wat"

func main(a : int, b : int) : void {
    var x : *int = [10]{1,2,3,4,5,6,7,8,9,10}

    var i : int = 0
    while(i < 10) {
        putn(*(x + 4 * i))
        i = i + 1
    }
}
