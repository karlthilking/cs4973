.global main

.data
msg:		.asciz " * 2 = "
newline:	.asciz "\n"

.text
main:
	li	t0, 0
	li	t1, 10
	
loop:
	li	a7, 1
	mv	a0, t0
	ecall
	li	a7, 4
	la	a0, msg
	ecall
	mv	a0, t0
	jal	func
	li	a7, 1
	mv 	a0, a0
	ecall
	li	a7, 4
	la 	a0, newline
	ecall
	addi	t0, t0, 1
	bgt 	t0, t1, end
	jal	loop
	
func:
	mv 	t2, a0
	li	t3, 2
	mul	t2, t2, t3
	mv 	a0, t2
	ret
	
end:
	li	a7, 10
	ecall