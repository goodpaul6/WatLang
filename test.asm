lis $27
.word 0xffff000c
lis $28
.word 72
sw $28, 0($27)
lis $28
.word 105
sw $28, 0($27)
lis $28
.word 10
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
lis $28
.word 10
sw $28, 0($27)
lis $27
.word 128
sub $30, $30, $27
add $27, $30, $0
add $1, $30, $0
L0:
lis $28
.word 0xffff0004
lw $29, 0($28)
lis $28
.word 10
beq $29, $28, L1
sw $29, 0($27)
lis $28
.word 4
add $27, $27, $28
lis $28
.word L0
jr $28
L1:
sw $0, 0($27)
add $27, $1, $0
L2:
lw $28, 0($27)
beq $28, $0, 9
lis $29
.word 0xffff000c
sw $28, 0($29)
lis $28
.word 1
add $27, $27, $28
lis $28
.word L2
jr $28
lis $28
.word 10
sw $28, 0($27)
lis $27
.word 1
lis $28
.word 2
add $27, $27, $28
add $2, $27, $0
add $27, $2, $0
L3:
lis $28
.word 10
div $27, $28
mflo $27
mfhi $29
lis $28
.word 48
add $28, $28, $29
lis $29
.word 0xffff000c
sw $28, 0($29)
beq $27, $0, 3
lis $28
.word L3
jr $28
lis $27
.word 10
lis $28
.word 0xffff000c
sw $27, 0($28)
jr $31
