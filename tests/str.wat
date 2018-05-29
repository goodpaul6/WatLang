#include "basic.wat"

func main() : void {
    var x : *char = [20]"Lorem ";
    strcat(x, "ipsum");

    puts(x);
}
