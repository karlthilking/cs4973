.global main

.data
out1:		.asciz "collatz("
out2:		.asciz "): "
newline: 	.asciz "\n"

.text
main:
	addi 	sp, sp, -8		# allocate space for two ints
	sw	s0, 0(sp)    	        # s0 is loop variable i
	sw	s1, 4(sp)		# s1 for loop condition i<100
	li	s0, 1   		# loop variable i=1
	li	s1, 100		        # loop condition i<100

# for (int i=1; i<100; i++)
#       printf("collatz(%d): %d\n", i, collatz(i));
loop:
	li	        a7, 4
	la	        a0, out1
	ecall                           # print "collatz("
	li	        a7, 1
	mv	        a0, s0          
	ecall	        		# print i (current number)
	li	        a7, 4
	la	        a0, out2        
	ecall	        		# print "): "
	mv	        a0, s0
	call	        collatz
	li	        a7, 1			
	ecall	        		# print collatz(i) (result)
	li	        a7, 4
	la 	        a0, newline
	ecall	        		# print "\n'"
	addi	        s0, s0, 1       # i++
	blt	        s0, s1, loop	# if i < 100 continue
					
# exit(0)
exit:
	li	        a7, 10
	ecall	

# collatz(n):
#       return f(n,1)
collatz:
	addi		sp, sp, -4 	# allocate space for ra
	sw		ra, 0(sp)	# store ra
	li		a1, 1           # x = 1
	call		f		# call f(n,1)
	lw		ra, 0(sp)    	# restore ra
	addi		sp, sp, 4	# deallocate stack
	ret				# return

# f(n,x):
#       if n == 1:      return x
#       if n % 2 == 0:  return f(n/2,x+1)
#       if n % 2 == 1:  return f(3*n+1,x+1)
f:
	li		t0, 1		# t0 = 1
	beq		a0, t0, fone	# if x == 1 (t0)
	addi		t0, t0, 1	# t0 = 2
	rem		t1, a0, t0	# t1 = x % 2 (t0)
	beqz		t1, feven	# if x % 2 (t1) == 0
	bnez		t1, fodd	# if x % 2 (t1) == 1

# if n == 1: return x
fone:
	mv		a0, a1		# store result (x) in a0
	ret				# return x (a0)

# if n % 2 == 0: return f(n/2,x+1)
feven:
	srli		a0, a0, 1	# n=n/2	
	addi		a1, a1, 1	# x=x+1
	j 		f		# call f(n/2,x+1)

# if n % 2 == 1: return f(3*n+1,x+1)
fodd:
	li	        t0, 3
	mul	        a0, a0, t0	# n=n*3
	addi 	        a0, a0, 1	# n=n+1
	addi	        a1, a1, 1	# x=x=1
	j	        f		# call f(3*n+1,x+1)
