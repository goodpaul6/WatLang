#include "basic.wat"

func main() : void {
    var a : *int = []{10, 20, 30, 40, 50};

    var i : int = 0;
    while(i < 5) {
        putn(*a);
        a = a + 4;
        i = i - -1;
    }
}
