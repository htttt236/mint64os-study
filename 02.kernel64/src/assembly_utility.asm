[bits 64]
SECTION .text


global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt
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


;========================테스크 관련 함수=========================

; 콘텍스트를 저장하고 셀렉터를 교체하는 매크로
%macro KSAVECONTEXT 0
    ; rbp 레지스터부터 gs 세그먼트 셀렉터까지 모두 스택에 삽입
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; ds, es는 스택에 직접 삽입 불가
    mov ax, ds
    push rax
    mov ax, es
    push rax
    push fs
    push gs
%endmacro

; 콘텍스트를 복원하는 매크로
%macro KLOADCONTEXT 0
    ; GS 세그먼트 셀렉터부터 RBP 레지스터까지 모두 스택에서 꺼내 복원
    pop gs
    pop fs

    ; es, ds는 직접 꺼내 복원 불가
    pop rax
    mov es, ax
    pop rax
    mov ds, ax
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
%endmacro

; Current Context에 현재 콘텍스트를 저장하고 Next Task에서 콘텍스트를 복구
; PARAM1: Current Context PARAM2: Next Context
kSwitchContext:
    push rbp
    mov rbp, rsp

    ; Current Context가 NULL이면 콘텍스트 저장할 필요 없음
    pushfq          ; 아래의 cmp의 결과로 RFLAGS 레지스터가 변하지 않도록 스택에 저장
    cmp rdi, 0      ; Current Context가 NULL이면 콘텍스트 복원으로 바로 이동
    je .LoadContext 
    popfq           ; 스택에 저장한 RFLAGS 레지스터를 복원

    ;=================현재 태스크의 콘텍스트 저장=========================
    push rax            ; 콘텍스트 영역의 오프셋으로 사용할 RAX 레지스터를 스택에 저장
    
    ; SS, RSP, RFLAGS, CS, RIP 레지스터 순서대로 삽입
    mov ax, ss                          ; SS 레지스터 저장
    mov qword[ rdi + ( 23 * 8 ) ], rax

    mov rax, rbp                        ; RBP에 저장된 RSP 레지스터 저장
    add rax, 16                         ; RSP 레지스터는 push rbp와 Return Address를
    mov qword[ rdi + ( 22 * 8 ) ], rax  ; 제외한 값으로 저장
    
    pushfq                              ; RFLAGS 레지스터 저장
    pop rax
    mov qword[ rdi + ( 21 * 8 ) ], rax

    mov ax, cs                          ; CS 레지스터 저장
    mov qword[ rdi + ( 20 * 8 ) ], rax
    
    mov rax, qword[ rbp + 8 ]           ; RIP 레지스터를 Return Address로 설정하여 
    mov qword[ rdi + ( 19 * 8 ) ], rax  ; 다음 콘텍스트 복원 시에 이 함수를 호출한 
                                        ; 위치로 이동하게 함
    
    ; 저장한 레지스터를 복구한 후 인터럽트가 발생했을 때처럼 나머지 콘텍스트를 모두 저장
    pop rax
    pop rbp
    
    ; 가장 끝부분에 SS, RSP, RFLAGS, CS, RIP 레지스터를 저장했으므로, 이전 영역에
    ; push 명령어로 콘텍스트를 저장하기 위해 스택을 변경
    add rdi, ( 19 * 8 )
    mov rsp, rdi
    sub rdi, ( 19 * 8 )
    
    ; 나머지 레지스터를 모두 Context 자료구조에 저장
    KSAVECONTEXT

    ;=================다음 태스크의 콘텍스트 복원=========================
.LoadContext:
    mov rsp, rsi

    ; Context 자료구조에서 레지스터를 복원
    KLOADCONTEXT
    iretq


; 프로세서를 쉬게 함
kHlt:
    hlt     ; 프로세서를 대기 상태로 진입시킴
    hlt
    ret