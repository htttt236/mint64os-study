disk_reset:
    pusha
    mov ah,0x00
    int 0x13

    jc disk_reset_error

    popa
    ret

disk_load:
    pusha
    
    mov ah, 0x42
    mov si, DAP

    int 0x13
    jc disk_load_error

    popa
    ret

disk_reset_error:
    add sp, 16
    mov bx, MSG_DISK_RESET_ERROR
    call print
    mov dx, 0
    mov dl, ah
    call print_hex

    jmp disk_error_loop

disk_load_error:
    add sp, 16
    mov bx, MSG_DISK_LOAD_ERROR
    call print
    mov dx, 0
    mov dl, ah
    call print_hex
    
    jmp disk_error_loop

disk_error_loop:
    jmp $

MSG_DISK_RESET_ERROR: db "Disk reset error:", 0
MSG_DISK_LOAD_ERROR: db "Disk read error:", 0

DAP:
    db 0x10
    db 0x00
    dw TOTAL_SECTOR_COUNT
    dw 0x0000
    dw 0x1000
    dd 0x00000001
    dd 0x00000000
