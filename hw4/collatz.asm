.global main

.data
out1:       .asciz "collatz("
out2:       .asciz "): "
newline:    .asciz "\n"

.text
main:
    addi    sp, sp, -8
    sw      s0, 0(sp)   
    sw      s1, 4(sp)
    li      s0, 1
    li      s1, 100

loop:
    li      a7, 4
    la      a0, out1
    ecall
    li      a7, 1
    mv      a0, s0
    ecall
    li      a7, 4
    la      a0, out2
    ecall
    li      a7, 1
    mv      a0, s0
    call    collatz
    ecall
    li      a7, 4
    la      a0, newline
    ecall
    addi    s0, s0, 1
    blt     s0, s1, loop

exit:
    li      a7, 10
    ecall

collatz:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    li      a1, 1
    call    f
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret

f:
    li      t0, 1
    beq     a0, t0, fone
    addi    t0, zero, 2
    rem     t1, a0, t0
    beqz    t1, feven
    bnez    t1, fodd

fone:
    mv      a0, a1
    ret

feven:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    srli    a0, a0, 1   # n=n/2
    addi    a1, a1, 1   # x=x+1
    call    f
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret

fodd:
    addi    sp, sp, -4
    sw      ra, 0(sp)
    li      t0, 3
    mul     a0, t0, a0  # n=3*n
    addi    a0, a0, 1   # n=n+1
    addi    a1, a1, 1   # x=x+1
    call    f
    lw      ra, 0(sp)
    addi    sp, sp, 4
    ret



