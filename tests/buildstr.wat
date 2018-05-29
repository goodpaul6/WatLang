#include "basic.wat"

func main() : void {
    var x : *char = [10]"";
    var i : int = 0;

    while(i < 8) {
        *(x + i * 4) = cast (char) (48 + i);
         i = i + 1;
    }

    puts(x);
}
