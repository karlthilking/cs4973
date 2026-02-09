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
    call    fib
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

fib:
    beqz    a0, fib0
    li      t0, 2
    ble     a0, t0, fible2

fibelse:
    addi    sp, sp, -16
    sw      ra, 0(sp)
    sw      s0, 4(sp)
    sw      s1, 8(sp)
    sw      s2, 12(sp)
    mv      s2, a0      # n
    addi    a0, s2, -1
    call    fib
    mv      s0, a0      # f(n-1)
    addi    a0, s2, -2
    call    fib         
    mv      s1, a0      # f(n-2)
    add     a0, s0, s1
    lw      ra, 0(sp)
    lw      s0, 4(sp)
    lw      s1, 8(sp)
    lw      s2, 12(sp)
    addi    sp, sp, 16
    ret

fib0:
    li      a0, 0
    ret

fible2:
    li      a0, 1
    ret



