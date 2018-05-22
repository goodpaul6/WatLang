#include "basic.wat"

func main(a, l) {
    var i = 0
    while(i < l) {
        putn(*a)
        a = a + 4
        i = i - -1
    }
}
