#!/bin/bash

for file in $(cat $1); do
    temp=$(mktemp)
    
    echo "Compiling ${file}"
    
    ./wat < "tests/${file}" > "bin/${file}.asm"
        
    echo "Assembling bin/${file}.asm"

    cs241.binasm < "bin/${file}.asm" > "bin/${file}.mips"

    echo "Running bin/${file}.mips"
    mips.twoints "bin/${file}.mips" < "tests/${file}.in" > "${temp}" 2> /dev/null

    echo "=============================================="
    
    cmp -s ${temp} "tests/${file}.out"

    if [ $? -ne 0 ]; then
        echo "Test failed: ${file}"
        echo "Input:"

        cat "tests/${file}.in"

        echo "Expected:"

        cat "tests/${file}.out"

        echo "Actual:"

        cat ${temp}
    fi

    rm ${temp}
done


