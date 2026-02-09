.global main

.data
out1: 	        .asciz "collatz("
out2: 	        .asciz "): "
newline: 	.asciz "\n"

.text
main:
	addi 	sp, sp, -8
	sw	s0, 0(sp)
	sw	s1, 4(sp)
	li	s0, 1
	li	s1, 100
	
loop:
	li	a7, 4
	la	a0, out1
	ecall
	li	a7, 1
	mv	a0, s0
	ecall
	li	a7, 4
	la	a0, out2
	ecall
	mv	a0, s0
	call	collatz
	li	a7, 1
	ecall
	li	a7, 4
	la 	a0, newline
	ecall
	addi	s0, s0, 1
	blt	s0, s1, loop
	
exit:
	li	a7, 10
	ecall	
	
collatz:
	addi	sp, sp, -4
	sw	ra, 0(sp)
	li	a1, 1
	call	f
	lw	ra, 0(sp)
	addi	sp, sp, 4
	jr	ra

f:
	li	t0, 1			
	beq	a0, t0, fone		# if x == 1
	addi	t0, t0, 1
	rem	t1, a0, t0
	beqz	t1, feven		# else if x % 2 == 0
	bnez	t1, fodd		# else if x % 2 == 1

# if (x==1) { return x; }
fone:
	mv	a0, a1
	ret

# else if (x%2==0) { return f(n/2,x+1); }
feven:
	srli	a0, a0, 1			
	addi	a1, a1, 1
	j 	f

# else if (x%2==1) { return f(3*n+1,x+1); }
fodd:
	li	t0, 3
	mul	a0, a0, t0
	addi 	a0, a0, 1
	addi	a1, a1, 1
	j	f
