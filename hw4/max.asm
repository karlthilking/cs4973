.global main

.data
prompt1:	.asciz "input a: "
prompt2:	.asciz "input b: "
out1:		.asciz "max("
out2:		.asciz "): "
comma: 	.asciz ","
newline:	.asciz "\n"

.text
main:
	li	a7, 4
	la	a0, prompt1
	ecall
	
	li	a7, 5		
	ecall
	mv	t0, a0
	
	li	a7, 4
	la	a0, prompt2
	ecall
	
	li	a7, 5
	ecall
	mv	t1, a0		
		
	li	a7, 4
	la	a0, out1
	ecall
	
	li 	a7, 1
	mv	a0, t0
	ecall
	
	li	a7, 4
	la	a0, comma
	ecall
	
	li	a7, 1
	mv	a0, t1
	ecall
	
	li	a7, 4
	la	a0, out2
	ecall
	
	mv	a0, t0
	mv	a1, t1
	call	max
	
	li	a7, 1
	mv	a0, a0
	ecall
	
	li	a7, 4
	la	a0, newline
	ecall
	
	li	a7, 10
	ecall
	
max:
	bge 	a0, a1, lmax
	b 	rmax
	
lmax:
	jr	ra
	
rmax:
	mv 	a0, a1
	jr 	ra