#include "basic.wat"

func main() : void {
    initAlloc();

    var a : *char = malloc(20 * 4);

    strcpy(a, "malloc");

    puts(a);

    free(a);
}
