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
; $1 = n
putn:
sw $31, -4($30)
lis $3
.word 4
sub $30, $30, $3
lis $3
.word 80
sub $30, $30, $3
add $4, $30, $0
lis $5
.word 10
putnstart:
beq $1, $0, putnprint
div $1, $5
mfhi $6
mflo $1
sw $6, 0($4)
lis $6
.word 4
add $4, $4, $6
lis $6
.word putnstart
jr $6
putnprint:
lis $5
.word 0xffff000c
putnprintloop:
beq $4, $30, putnend
lw $6, -4($4)
lis $7
.word 48
add $6, $6, $7
sw $6, 0($5)
lis $6
.word 4
sub $4, $4, $6
lis $6
.word putnprintloop
jr $6
putnend:
lis $3
.word 80
add $30, $30, $3
sw $1, -4($30)
sw $2, -8($30)
sw $3, -12($30)
lis $1
.word 12
sub $30, $30, $1
lis $3
.word 10
add $1, $3, $0
lis $3
.word putc
jalr $3
add $4, $2, $0
lis $5
.word 12
add $30, $30, $5
lw $3, -12($30)
lw $2, -8($30)
lw $1, -4($30)
lis $4
.word 4
add $30, $30, $4
lw $31, -4($30)
jr $31
main:
sw $31, -4($30)
lis $2
.word 4
sub $30, $30, $2
sw $1, -4($30)
sw $2, -8($30)
lis $1
.word 8
sub $30, $30, $1
lis $4
.word 123
lis $5
.word 100
sub $3, $4, $5
add $1, $3, $0
lis $3
.word putn
jalr $3
add $3, $2, $0
lis $4
.word 8
add $30, $30, $4
lw $2, -8($30)
lw $1, -4($30)
sw $1, -4($30)
sw $2, -8($30)
lis $1
.word 8
sub $30, $30, $1
lis $3
.word 250
add $1, $3, $0
lis $3
.word putn
jalr $3
add $3, $2, $0
lis $4
.word 8
add $30, $30, $4
lw $2, -8($30)
lw $1, -4($30)
lis $6
.word 4
add $30, $30, $6
lw $31, -4($30)
jr $31
