.global main

.data
out1:       .asciz "collatz("
out2:       .asciz "): "
newline:    .asciz "\n"

.text
main:
    addi    sp, sp, -8
    sw      s0, 0(sp)   # loop variable i
    sw      s1, 4(sp)   # number of iterations n
    li      s0, 1       # i=1
    li      s1, 100     # n=100

# for (i=0; i<100; i++)
#   print("collatz(%d): %d\n", i, collatz(i));
loop:
    li      a7, 4
    la      a0, out1
    ecall                   # print "collatz("
    li      a7, 1
    mv      a0, s0
    ecall                   # print %d, i
    li      a7, 4
    la      a0, out2
    ecall                   # print "): "
    li      a7, 1
    mv      a0, s0
    call    collatz
    ecall                   # print %d, collatz(i)
    li      a7, 4
    la      a0, newline
    ecall                   # print "\n"
    addi    s0, s0, 1       # i++
    blt     s0, s1, loop    # if i<n(100): continue

# exit(0)
exit:
    li      a7, 10
    ecall

# collatz(n)
#   return f(n,1)
collatz:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    li      a1, 1
    call    f               # call f(n,1)
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret                     # return f(n,1)

# f(n,x)
#   if n==1:    return x
#   if n%2==0:  return f(n/2,x+1)
#   if n%2==1:  return f(3*n+1,x+1)
#
f:
    li      t0, 1
    beq     a0, t0, fone    # if n==1
    addi    t0, zero, 2     
    rem     t1, a0, t0      
    beqz    t1, feven       # if n%2==0
    bnez    t1, fodd        # if n%2==1

# if n==1: return x
fone:
    mv      a0, a1          
    ret                     # return x

# if n%2==0: return f(n/2,x+1)
feven:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    srli    a0, a0, 1       # n=n/2
    addi    a1, a1, 1       # x=x+1
    call    f               # call f(n/2,x+1)
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret                     # return f(n/2,x+1)

# if n%2==1: return f(3*n+1,x+1)
fodd:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    li      t0, 3
    mul     a0, t0, a0      # n=3*n
    addi    a0, a0, 1       # n=n+1
    addi    a1, a1, 1       # x=x+1
    call    f               # call f(3*n+1,x+1)
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret                     # return f(3*n+1,x+1)



