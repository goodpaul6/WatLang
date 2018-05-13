lis $27
.word 0xffff000c
lis $28
.word 72
sw $28, 0($27)
lis $28
.word 105
sw $28, 0($27)
lis $27
.word 0xffff000c
lis $28
.word 78
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
.word 58
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
.word 30
jr $28
jr $31
