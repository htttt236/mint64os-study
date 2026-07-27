%ifndef GDT_INC
%define GDT_INC

gdt_start:
    dd 0x00000000
    dd 0x00000000

gdt_code_IA_32e:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 10101111b
    db 0x00

gdt_data_IA_32e:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 10101111b
    db 0x00

gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start - $$ + 0x10000

%endif