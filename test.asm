lis $1
.word main
jr $1
; globals
; code
; $1 = c
putc:
sw $31, -4($30)
lis $3
.word 4
sub $30, $30, $3
lis $3
.word 0xffff000c
sw $1, 0($3)
lis $2
.word 4
add $30, $30, $2
lw $31, -4($30)
jr $31
main:
sw $31, -4($30)
lis $2
.word 4
sub $30, $30, $2
sw $1, -4($30)
sw $2, -8($30)
lis $3
.word 8
sub $30, $30, $3
lis $3
.word 48
add $1, $3, $0
lis $3
.word putc
jalr $3
add $3, $2, $0
lis $4
.word 8
add $30, $30, $4
lw $2, -8($30)
lw $1, -4($30)
sw $1, -4($30)
sw $2, -8($30)
lis $3
.word 8
sub $30, $30, $3
lis $3
.word 10
add $1, $3, $0
lis $3
.word putc
jalr $3
add $3, $2, $0
lis $4
.word 8
add $30, $30, $4
lw $2, -8($30)
lw $1, -4($30)
lis $4
.word 4
add $30, $30, $4
lw $31, -4($30)
jr $31
