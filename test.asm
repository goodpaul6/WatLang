lis $27
.word 0xffff000c
lis $28
.word 65
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
sw $25, -4($30)
sw $26, -8($30)
lis $25
.word 8
sub $30, $30, $25
add $25, $0, $0
beq $27, $1, L2
lis $26
.word 1
lis $28
.word 4
sub $27, $27, $28
L3:
lw $29, 0($27)
lis $28
.word 45
bne $29, $28, L4
sub $25, $0, $25
lis $28
.word L2
jr $28
L4:
lis $28
.word 48
sub $29, $29, $28
mult $29, $26
mflo $29
add $25, $25, $29
beq $27, $1, L2
lis $28
.word 4
sub $27, $27, $28
lis $28
.word 10
mult $26, $28
mflo $26
lis $28
.word L3
jr $28
L2:
add $27, $25, $0
lis $25
.word 8
add $30, $30, $25
lw $26, -8($30)
lw $25, -4($30)
add $1, $27, $0
add $27, $1, $0
L5:
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
beq $27, $0, L6
lis $28
.word L5
jr $28
L6:
lis $27
.word 10
lis $28
.word 0xffff000c
sw $27, 0($28)
jr $31
