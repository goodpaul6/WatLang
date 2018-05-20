lis $1
.word L0
jr $1
; globals
.word 0
L0:
; code
lis $1
.word 10
; storing into x
sw $1, 12($0)
L1:
lw $2, 12($0)
lis $3
.word 0
slt $1, $2, $3
lis $4
.word 1
sub $1, $4, $1
bne $2, $3, 1
add $1, $0, $0
beq $1, $0, L2
lw $3, 12($0)
lis $4
.word 1
sub $2, $3, $4
; storing into x
sw $2, 12($0)
lis $2
.word L1
jr $2
L2:
jr $31
