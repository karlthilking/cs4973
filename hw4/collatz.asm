.global main

.data
out1:       .asciz "collatz("
out2:       .asciz "): "
newline:    .asciz "\n"
out3:       .asciz ", collatziter("


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
    la      a0, out3 
    ecall
    li      a7, 1
    mv      a0, s0
    ecall
    li      a7, 4
    la      a0, out2
    ecall
    li      a7, 1
    mv      a0, s0
    call    collatziter
    ecall
    li      a7, 4
    la      a0, newline
    ecall
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

# iterative collatz function
# collatziter(n):
#   for(x=0;; x++)
#       if n==1:    break
#       if n%2==0:  n=n/2
#       if n%2==1:  n=3*n+1
#   return x
collatziter:
    addi    sp, sp, -12
    sw      s0, 0(sp)
    sw      s1, 4(sp)
    sw      s2, 8(sp)
    li      s0, 1           # x (number of iterations)
    li      s1, 1           
    li      s2, 2

collatzloop:
    beq     a0, s1, collatzret
    rem     t0, a0, s2      # n%2
    beqz    t0, collatzeven
    bnez    t0, collatzodd

collatzeven:
    srli    a0, a0, 1       # n=n/2
    addi    s0, s0, 1       # x=x+1
    j       collatzloop

collatzodd:
    li      t0, 3
    mul     a0, t0, a0      # n=3*n
    addi    a0, a0, 1       # n=n+1
    addi    s0, s0, 1       # x=x+1
    j       collatzloop

collatzret:
    mv      a0, s0
    lw      s0, 0(sp)
    lw      s1, 4(sp)
    lw      s2, 8(sp)
    addi    sp, sp, 12
    ret

