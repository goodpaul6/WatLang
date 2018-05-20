; globals
lis $1
.word L0
jr $1
.word 0
.word 0
L0:
; code
lis $2
.word 10
lis $4
.word 20
lis $5
.word 30
mult $4, $5
mflo $3
mult $2, $3
mflo $1
; storing into x
sw $1, 0($0)
lw $3, 0($0)
lis $4
.word 10
mult $3, $4
mflo $2
lis $3
.word 5
add $1, $2, $3
; storing into y
sw $1, 4($0)
jr $31
