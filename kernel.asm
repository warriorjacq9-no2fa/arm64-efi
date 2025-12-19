.section .text
.global _start

_start:
    /* Set up stack (PC-relative) */
    adrp x1, stack_top
    add  x1, x1, :lo12:stack_top
    mov  sp, x1
    
    mov x19, x0

        /* UART base */
    movz x0, #0x0000
    movk x0, #0x0900, lsl #16   /* 0x09000000 */

    /* msg pointer */
    adrp x1, msg
    add  x1, x1, :lo12:msg

1:
    ldrb w2, [x1], #1
    cbz  w2, 3f

/* wait for TX FIFO not full */
2:
    ldr  w3, [x0, #0x18]    /* UARTFR */
    tst  w3, #(1 << 5)      /* TXFF */
    b.ne 2b

    strb w2, [x0]           /* UARTDR */
    b    1b

3:
halt:
    wfe
    b halt

/* -------------------------------------------------- */

.section .rodata
msg:
    .asciz "Hello from ARM64!\r\n"

/* -------------------------------------------------- */

.section .bss
.align 16
stack:
    .skip 0x4000
stack_top:
