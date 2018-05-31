#include "basic.wat"

func main() : void {
    assert(strcmp("hello", "hello") == 0, "hello was not hello.");
    assert(strcmp("What", "what") == 0, "Good.");
}
