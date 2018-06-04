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

func main() : void {
    setX(10);
    setY(-30);

    putn(addXY());
}
