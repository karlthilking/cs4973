.global main

.data
ouput: 	.asciz "Number: "
newline:	.asciz "\n"


.text
main:
	li	t0, 0
	li	t1, 1
	slli	t1, t1, 30
	
loop:
	addi 	t0, t0, 1
	beq 	t1, zero, end
	li 	a7, 4
	la	a0, ouput 
	ecall
	li 	a7, 1
	mv 	a0, t1
	ecall
	li 	a7, 4
	la 	a0, newline
	ecall
	srli 	t1, t1, 1
	b 	loop
	
end:
	li 	a7, 10
	ecall