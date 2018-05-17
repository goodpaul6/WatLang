#!/bin/bash

temp=$(mktemp)
tempAsm=$(mktemp)
tempMips=$(mktemp)

while true; do
    read line
    if [ "$line" == "RUN" ]; then
        $1 < $temp > $tempAsm
        cs241.binasm < $tempAsm > $tempMips
        mips.twoints $tempMips
    else
        echo line > $temp
    fi
done
