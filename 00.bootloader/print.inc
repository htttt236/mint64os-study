clear_display:
    push ax
    push di
    push es
    mov ax, 0xb800
    mov es, ax
    mov di, 0

clear_display_loop:
    mov byte [es:di], 0
    mov byte [es:di+1], 0x07

    add di, 2
    cmp di, 80*25*2
    jl clear_display_loop

    pop es
    pop di
    pop ax
    ret


print:
    push bp
    mov bp, sp
    pusha
    push es

    ; [bp+4] = 80*Y+X
    mov ax, 0xb800
    mov es, ax
    mov ax, [bp+4]
    imul ax, 2
    mov di, ax

print_loop:
    mov al, [bx]

    cmp al, 0
    je print_end

    mov byte [es:di], al
    add di, 2
    add bx, 1

    jmp print_loop

print_end:
    mov dx, 0
    mov bx, 2
    mov ax, di
    div bx
    mov [bp+4], ax
    pop es
    popa
    pop bp
    ret


print_hex:
    push bp
    mov bp, sp
    pusha

    mov cx, 0

print_hex_loop:
    cmp cx, 4
    je print_hex_end

    mov ax, dx
    and ax, 0x000f
    add al, 0x30
    cmp al, 0x39
    jle step2
    add al, 39

step2:
    mov bx, HEX_OUT + 5
    sub bx, cx
    mov byte [bx], al
    ror dx, 4

    add cx, 1
    jmp print_hex_loop

print_hex_end:
    mov bx, HEX_OUT
    push word [bp+4]
    call print

    pop ax
    mov [bp+4], ax
    
    popa
    pop bp
    ret

HEX_OUT:
    db "0x0000", 0