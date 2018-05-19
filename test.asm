lis $2
.word 10
lis $5
.word 20
lis $6
.word 3
mult $5, $6
mflo $4
lis $6
.word 2
lis $7
.word 4
mult $6, $7
mflo $5
add $3, $4, $5
add $1, $2, $3
jr $31
