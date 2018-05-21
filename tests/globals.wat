#include "basic.wat"

var x
var y

func setX(n) {
    x = n
}

func setY(n) {
    y = n
}

func addXY() {
    return x + y
}

func main(a, b) {
    setX(a)
    setY(b)
    putn(addXY())
}
