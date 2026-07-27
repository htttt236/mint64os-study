[bits 16]

SECTION .text

start:
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    ; A20 게이트 활성화
    mov ax, 0x2401
    int 0x15

    jnc switch_to_pm

    in al, 0x92
    or al, 0x02
    and al, 0xfe
    out 0x92, al

switch_to_pm:
    cli
    lgdt [gdt_descriptor - $$ + 0x10000]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp dword CODE_SEG:(init_pm - $$ + 0x10000)


[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0xfffc
    mov esp, ebp

    mov ebx, MSG_PM_SUCCESS - $$ + 0x10000
    push 160
    call pm_print
    add esp, 4

    extern main
    call main

    jmp $

%include "gdt.inc"
%include "gdt_seg.inc"
%include "32bit_print.inc"

MSG_PM_SUCCESS db "Landed in 32-bit Protected Mode", 0