#include "basic.wat"

func main() : void {
    if(true && true) {
        puts("true && true");
    }

    if(true && false) {
        puts("true && false");
    }

    if(false && true) {
        puts("false && true");
    }

    if(true || true) {
        puts("true || true");
    }

    if(true || false) {
        puts("true || false");
    }

    if(false || true) {
        puts("false || true");
    }

    if(false || false) {
        puts("false || false");
    }
}
