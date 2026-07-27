extern main

start:
    mov ax, DATA_SEG_IA_32e
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov rsp, 0x6ffff8
    mov rbp, 0x6ffff8

    call main

    jmp $

%include "gdt_seg.inc"