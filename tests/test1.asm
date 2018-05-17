lis $27
.word 0xffff000c
lis $28
.word 72
sw $28, 0($27)
lis $28
.word 105
sw $28, 0($27)
lis $28
.word 33
sw $28, 0($27)
lis $28
.word 87
sw $28, 0($27)
lis $28
.word 104
sw $28, 0($27)
lis $28
.word 97
sw $28, 0($27)
lis $28
.word 116
sw $28, 0($27)
lis $28
.word 39
sw $28, 0($27)
lis $28
.word 115
sw $28, 0($27)
lis $28
.word 121
sw $28, 0($27)
lis $28
.word 111
sw $28, 0($27)
lis $28
.word 117
sw $28, 0($27)
lis $28
.word 114
sw $28, 0($27)
lis $28
.word 110
sw $28, 0($27)
lis $28
.word 97
sw $28, 0($27)
lis $28
.word 109
sw $28, 0($27)
lis $28
.word 101
sw $28, 0($27)
lis $28
.word 63
sw $28, 0($27)
lis $27
.word 32
sub $30, $30, $27
add $1, $30, $0
add $27, $30, $0
lis $28
.word 0xffff0004
lw $29, 0($28)
lis $28
.word 10
beq $29, $28, 7
sw $29, 0($27)
lis $28
.word 1
add $27, $27, $28
lis $28
.word 61
jr $28
sw $0, 0($27)
add $27, $1, $0
lw $28, 0($27)
beq $28, $0, 9lis $29
.word 0xffff000c
sw $28, 0($29)
lis $28
.word 1
add $27, $27, $28
lis $28
.word 76
jr $28
lis $27
.word 0xffff000c
lis $28
.word 67
sw $28, 0($27)
lis $28
.word 111
sw $28, 0($27)
lis $28
.word 111
sw $28, 0($27)
lis $28
.word 108
sw $28, 0($27)
lis $28
.word 110
sw $28, 0($27)
lis $28
.word 97
sw $28, 0($27)
lis $28
.word 109
sw $28, 0($27)
lis $28
.word 101
sw $28, 0($27)
lis $28
.word 33
sw $28, 0($27)
jr $31
