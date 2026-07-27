global kInPortByte, kOutPortByte
; 포트로브터 1바이트 읽기
; param1: 포트 번호
kInPortByte:
    push rdx

    mov rdx, rdi
    xor rax, rax
    in al, dx

    pop rdx
    ret

; 포트에 1바이트 쓰기
; param1: 포트 번호 param2: 데이터
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al

    pop rax
    pop rdx
    ret