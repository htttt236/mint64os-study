[bits 64]
SECTION .text


global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
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


kLoadGDTR:
    lgdt [rdi]
    ret

kLoadTR:
    ltr di
    ret

kLoadIDTR:
    lidt [rdi]
    ret

; 인터럽트 활성화
kEnableInterrupt:
    sti
    ret

; 인터럽트 비활성화
kDisableInterrupt:
    cli
    ret

; RFLAGS 레지스터 읽기
kReadRFLAGS:
    pushfq
    pop rax
    ret

; 타임 스탬프 카운터를 읽어서 반환
kReadTSC:
    push rdx

    rdtsc       ; 타임 스탬프 카운터를 읽어 rdx에 상위 32비트, rax에 하위 32비트 저장

    shl rdx, 32
    or rax, rdx ; 두 값을 합쳐 rax에 저장

    pop rdx
    ret