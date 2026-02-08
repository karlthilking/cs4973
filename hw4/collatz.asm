.global main

.data
out1: 	.asciz "collatz("
out2: 	.asciz "): "
newline: 	.asciz "\n"

.text
main:
	li 	t0, 0	
	li 	t1, 100
	
loop:
	addi	t0, t0, 1
	mv 	a0, t0
	call 	collatz

collatz:
	mv 	a0, a0
	li 	a1, 1
	call 	f
f:


feven:

fodd:

end:
	li	a7, 10
	ecall