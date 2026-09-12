[bits 64]
SECTION .text


global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt, kTestAndSet
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS

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

; 테스트와 설정을 하나의 명령으로 처리
;	Destination과 Compare를 비교하여 같다면, Destination에 Source 값을 삽입
;   PARAM: 값을 저장할 어드레스(Destination, rdi), 비교할 값(Compare, rsi), 
;          설정할 값(Source, rdx) 
kTestAndSet:
    mov rax, rsi    ; 두 번째 파라미터인 비교할 값을 rax 레지스터에 저장

    ; rax 레지스터에 저장된 비교할 값과 첫 번째 파라미터의 메모리 어드레스의 값을
    ; 비교하여 두 값이 같다면 세 번째 파라미터의 값을 첫 번째 파라미터가 가리키는
    ; 어드레스에 삽입
    lock cmpxchg byte [rdi], dl
    je .SUCCESS

.NOTSAME:           ; destination과 compare가 다른 경우
    mov rax, 0x00
    ret

.SUCCESS:           ; destination과 compare가 같은 경우
    mov rax, 0x01
    ret


;==================FPU 관련 어셈블리어 함수=====================

; FPU를 초기화
kInitializeFPU:
    finit           ;FPU 초기화를 수행
    ret

; FPU 관련 레지스터를 콘텍스트 버퍼에 저장
; PARAM: BUffer Address
kSaveFPUContext:
    fxsave [rdi]    ; 첫 번째 파라미터로 전달된 버퍼에 FPU 레지스터를 저장
    ret

; FPU 관련 레지스터를 콘텍스트 버퍼에서 복원
; PARAM: Buffer Address
kLoadFPUContext:
    fxrstor [rdi]   ; 첫 번째 파라미터로 전달된 버퍼에서 FPU 레지스터를 복원
    ret

; cr0 컨트롤 레지스터의 TS 비트를 1로 설정
kSetTS:
    push rax        ; 스택에 rax 값을 저장

    mov rax, cr0    ; cr0의 값을 rax 레지스터로 저장
    or rax, 0x08    ; TS 비트(비트 7)를 1로 설정
    mov cr0, rax    ; TS 비트가 1로 설정된 값을 cr0로 저장

    pop rax         ; 스택에서 rax 값을 복원
    ret

; cr0 컨트롤 레지스터의 TS비트를 0으로 설정
kClearTS:
    clts            ; cr0에서 TS비트를 0으로 설정
    ret