[org 0x7c00]
[bits 16]
%ifndef KERNEL_SECTOR_COUNT
    %define KERNEL_SECTOR_COUNT 2
%endif
TOTAL_SECTOR_COUNT equ KERNEL_SECTOR_COUNT
jmp 0x0000:start
start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov bp, 0x9000
    mov sp, bp

    call clear_display
    mov bx, MSG_BOOTLOADER_START
    push 0
    call print
    add sp, 2

    push 80
    call disk_reset

    mov bx, MSG_DISK_LOAD
    call print
    call disk_load

    mov bx, MSG_DISK_LOAD_SUCCESS
    call print
    add sp, 2

    jmp 0x1000:0x0000

%include "print.inc"
%include "disk.inc"

MSG_BOOTLOADER_START db "Bootloader Start", 0
MSG_DISK_LOAD db "Disk loading... ", 0
MSG_DISK_LOAD_SUCCESS db "Success", 0

times 510-($-$$) db 0
dw 0xaa55