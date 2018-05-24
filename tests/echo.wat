#include "basic.wat"

func main() : void {
    var len : int = 0
    var s : *char = [256] ""

    var c : char = getc()
    
    while(c != cast (char) 10) {
        if(len >= 255) {
            puts(s)
            return;
        }

        *(s + len * 4) = c
        len = len + 1

        c = getc()
    }

    puts(s)
}
