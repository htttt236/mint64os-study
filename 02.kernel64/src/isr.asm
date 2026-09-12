[bits 64]

SECTION .text

GDT_KERNELDATASEGMENT equ 0x10
extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler
extern kTimerHandler, kDeviceNotAvailableHandler

global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable
global kISRDoubleFault, kISRCoprocessorSegmentOverrun, kISRInvalidTSS, kISRSegmentNotPresent
global kISRStackSegmentFault, kISRGeneralProtection, kISRPageFault, kISR15
global kISRFPUError, kISRAlignmentCheck, kISRMachineCheck, kISRSIMDError
global kISRVirtualizationException, kISRControlProtectionException, kISRETCException
global kISRHypervisorInjectionException, kISRVMMCommunicationException
global kISRSecurityException, kISR31

global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1, kISRParallel2
global kISRFloppy, kISRParallel1, kISRRTC, kISRReserved, kISRNotUsed1, kISRNotUsed2
global kISRMouse, kISRCoprocessor, kISRHDD1, kISRHDD2, kISRETCInterrupt

; 콘텍스트 저장하고 셀렉터 교체하는 매크로
%macro KSAVECONTEXT 0
    push rbp
    mov rbp, rsp
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

    ; ds부터 fs까지 커널 데이터 세그먼트로 교체
    mov ax, GDT_KERNELDATASEGMENT
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
%endmacro

; 콘텍스트 복원하는 매크로
%macro KLOADCONTEXT 0
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

;========예외 핸들러==================================

; #0, Divide Error
kISRDivideError:
    KSAVECONTEXT

    mov rdi, 0
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #1, Debug ISR
kISRDebug:
    KSAVECONTEXT

    mov rdi, 1
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #2, NMI ISR
kISRNMI:
    KSAVECONTEXT

    mov rdi, 2
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #3, BerakPoint ISR
kISRBreakPoint:
    KSAVECONTEXT

    mov rdi, 3
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #4, Overflow ISR
kISROverflow:
    KSAVECONTEXT

    mov rdi, 4
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #5, Bound Range Exceeded ISR
kISRBoundRangeExceeded:
    KSAVECONTEXT

    mov rdi, 5
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #6, Invalid Opcode ISR
kISRInvalidOpcode:
    KSAVECONTEXT

    mov rdi, 6
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #7, Device Not Available ISR
kISRDeviceNotAvailable:
    KSAVECONTEXT    ; 콘텍스트를 저장한 뒤 셀렉터를 커널 데이터 디스크립터로 교체

    ; 핸들러에 예외 번호를 삽입하고 핸들러 호출
    mov rdi, 7
    call kDeviceNotAvailableHandler

    KLOADCONTEXT    ; 콘텍스트를 복원
    iretq           ; 인터럽트 처리를 완료하고 이전에 수행하던 코드로 복원

; #8, Double Fault ISR
kISRDoubleFault:
    KSAVECONTEXT

    mov rdi, 8
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #9, Coprocessor Segment Overrun ISR
kISRCoprocessorSegmentOverrun:
    KSAVECONTEXT

    mov rdi, 9
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #10, Invalid TSS ISR
kISRInvalidTSS:
    KSAVECONTEXT

    mov rdi, 12
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #11, Segment Not Present ISR
kISRSegmentNotPresent:
    KSAVECONTEXT

    mov rdi, 11
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #12, Stack Segment Fault ISR
kISRStackSegmentFault:
    KSAVECONTEXT

    mov rdi, 10
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #13, General Protection ISR
kISRGeneralProtection:
    KSAVECONTEXT

    mov rdi, 13
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #14, Page Fault ISR
kISRPageFault:
    KSAVECONTEXT

    mov rdi, 14
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #15, Reserved ISR
kISR15:
    KSAVECONTEXT

    mov rdi, 15
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #16, FPU Error ISR
kISRFPUError:
    KSAVECONTEXT

    mov rdi, 16
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #17, Alignment Check ISR
kISRAlignmentCheck:
    KSAVECONTEXT

    mov rdi, 17
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #18, Machine Check ISR
kISRMachineCheck:
    KSAVECONTEXT

    mov rdi, 18
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #19, SIMD Floating Point Exception ISR
kISRSIMDError:
    KSAVECONTEXT

    mov rdi, 19
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #20, Virtualization Exception ISR
kISRVirtualizationException:
    KSAVECONTEXT

    mov rdi, 20
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #21, Control Protection Exception ISR
kISRControlProtectionException:
    KSAVECONTEXT

    mov rdi, 21
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #22~#27, Reserved ISR
kISRETCException:
    KSAVECONTEXT

    mov rdi, 22
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #28, Hypervisor Injection Exception ISR
kISRHypervisorInjectionException:
    KSAVECONTEXT

    mov rdi, 28
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #29, VMM Communication Exception ISR
kISRVMMCommunicationException:
    KSAVECONTEXT

    mov rdi, 29
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq

; #30, Security Exception ISR
kISRSecurityException:
    KSAVECONTEXT

    mov rdi, 30
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler

    KLOADCONTEXT
    add rsp, 8
    iretq

; #31, Resrved ISR
kISR31:
    KSAVECONTEXT

    mov rdi, 31
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq
    

;========인터럽트 핸들러================================
; #32, 타이머 ISR
kISRTimer:
    KSAVECONTEXT

    mov rdi, 32
    call kTimerHandler

    KLOADCONTEXT
    iretq

; #33, 키보드 ISR
kISRKeyboard:
    KSAVECONTEXT

    mov rdi, 33
    call kKeyboardHandler

    KLOADCONTEXT
    iretq

; #34, 슬레이브 PIC ISR
kISRSlavePIC:
    KSAVECONTEXT

    mov rdi, 34
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #35, 시리얼 포트 2 ISR
kISRSerial2:
    KSAVECONTEXT

    mov rdi, 35
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #36, 시리얼 포트 1 ISR
kISRSerial1:
    KSAVECONTEXT

    mov rdi, 36
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #37, 패러렐 포트 2 ISR
kISRParallel2:
    KSAVECONTEXT

    mov rdi, 37
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #38, 플로피 디스크 컨트롤러 ISR
kISRFloppy:
    KSAVECONTEXT

    mov rdi, 38
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #39, 패러렐 포트 1 ISR
kISRParallel1:
    KSAVECONTEXT

    mov rdi, 39
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #40, RTC ISR
kISRRTC:
    KSAVECONTEXT

    mov rdi, 40
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #41, 예약된 인터럽트의 ISR
kISRReserved:
    KSAVECONTEXT

    mov rdi, 41
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #42, 사용 안함
kISRNotUsed1:
    KSAVECONTEXT

    mov rdi, 42
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #43, 사용 안함
kISRNotUsed2:
    KSAVECONTEXT

    mov rdi, 43
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #44, 마우스 ISR
kISRMouse:
    KSAVECONTEXT

    mov rdi, 44
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #45, 코프로세서 ISR
kISRCoprocessor:
    KSAVECONTEXT

    mov rdi, 45
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #46, 하드 디스크 1 ISR
kISRHDD1:
    KSAVECONTEXT

    mov rdi, 46
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #47, 하드 디스크 2 ISR
kISRHDD2:
    KSAVECONTEXT

    mov rdi, 47
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq

; #48 이외의 모든 인터럽트에 대한 ISR
kISRETCInterrupt:
    KSAVECONTEXT

    mov rdi, 48
    call kCommonInterruptHandler

    KLOADCONTEXT
    iretq