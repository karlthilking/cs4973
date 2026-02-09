.global main

.data
out1:       .asciz "fib("
out2:       .asciz "): "
newline:    .asciz "\n"

.text
main:
    addi    sp, sp, -8
    sw      s0, 0(sp)
    sw      s1, 4(sp)
    li      s0, 0
    li      s1, 10

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
    mv      a0, s0
    call    fibiter
    li      a7, 1
    ecall
    li      a7, 4
    la      a0, newline
    ecall
    addi    s0, s0, 1
    ble     s0, s1, loop

end:
    li      a7, 10
    ecall

fibiter:
    beqz    a0, fib0
    li      t0, 3
    blt     a0, t0, fiblte2
    addi    sp, sp, -8
    sw      s2, 0(sp)
    sw      s3, 4(sp)
    li      s2, 0
    mv      s3, a0
    li      t0, 0
    li      t1, 1

fibloop:
    mv      t2, t1
    add     t1, t1, t0
    add     t0, t2, zero
    addi    s2, s2, 1
    blt     s2, s3, fibloop

fibret:
    mv      a0, t0
    ret

fib0:
    li      a0, 0
    ret

fiblte2:
    li      a0, 1
    ret
    



