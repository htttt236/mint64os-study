[bits 32]
VIDEO_MEEMORY equ 0xb8000
pm_print:
    push ebp
    mov ebp, esp
    pushad

    ; [bp+8] = 80*Y+X
    mov eax, [ebp+8]
    imul eax, 2

    add eax, 0xB8000
    mov edi, eax

pm_print_loop:
    mov al, [ebx]

    cmp al, 0
    je pm_print_end

    mov byte [edi], al
    add edi, 2
    add ebx, 1

    jmp pm_print_loop

pm_print_end:
    mov eax, edi
    sub eax, 0xb8000
    mov edx, 0
    mov ecx, 2
    div ecx

    mov [ebp+8], eax
    
    popad
    pop ebp
    ret