.global main

.data
loopmsg: .asciz "count: "
newline: .asciz "\n"
exitmsg: .asciz "exit\n"

.text
main:
    li t0, 0
    ti t1, 10

loop:
    addi t0, t0, 1
    li a7, 4        # 4: print string
    la a0, loopmsg
    ecall
    li a7, 1        # 1: print int
    li a0, t0       
    ecall
    bge t0, t1, end


end:
    li a7, 4
    la a0, exitmsg
    ecall
    li a7, 10
    ecall

