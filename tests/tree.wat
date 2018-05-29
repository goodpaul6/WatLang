#include "../WatLang/basic.wat"

func traverse(ap : *int, index : int) : void {
    if(index < 0) {
        return;
    }

    var np : *int = ap + index * 4;

    putn(*np);

    traverse(ap, *(np + 4));
    traverse(ap, *(np + 8));
}

func main() : void {
    var ap : *int = []{10,-1,3,5,-1,6,2,-1,-1};

    traverse(ap, 0);
}
