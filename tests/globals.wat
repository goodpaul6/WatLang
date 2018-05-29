#include "basic.wat"

var x : int;
var y : int;

func setX(n : int) : void {
    x = n;
}

func setY(n : int) : void {
    y = n;
}

func addXY() : int {
    return x + y;
}

func main(a : int, b : int) : void {
    setX(a);
    setY(b);
    putn(addXY());
}
