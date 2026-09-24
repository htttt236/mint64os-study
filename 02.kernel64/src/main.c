#include "types.h"
#include "keyboard.h"
#include "descriptor.h"
#include "assembly_utility.h"
#include "utility.h"
#include "pic.h"
#include "console.h"
#include "console_shell.h"
#include "task.h"
#include "pit.h"
#include "dynamic_memory.h"


void main(){

    // 콘솔 초기화 후 작업 수행
    kInitializeConsole(0, 10);

    kPrintf("%s","switch to IA-32e mode\n");
    kPrintf("IA-32e C language kernel start\n");

    kPrintf("GDT initialize and switch for IA-32e mode......");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kPrintf("pass\n");

    kPrintf("TSS segment load......");
    kLoadTR(GDT_TSSSEGMENT);
    kPrintf("pass\n");

    kPrintf("IDT nitialize......");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kPrintf("pass\n");

    kPrintf("total RAM size check......");
    kCheckTotalRAMSize();
    kPrintf("pass, size = %d MB\n", kGetTotalRAMSize());

    kPrintf("TCB pool and scheduler initialize......");
    kInitializeScheduler();
    kPrintf("pass\n");
    // 1ms당 한 번씩 인터럽트가 발생하도록 설정
    kInitializePIT(MSTOCOUNT(1), 1);

    // 동적 메모리 초기화
    kPrintf("dynamic memory initialize......");
    kInitializeDynamicMemory();
    kPrintf("pass\n");

    kPrintf("keyboard activate and queue initialize......");
    // 키보드 활성화
    if(kInitializeKeyboard() == true){
        kPrintf("pass\n");
        kChangeKeyboardLED(false, true, false);
    }
    else{
        kPrintf("fail");
        while(true);
    }

    kPrintf("PIC controller and interrupt initialize......");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kPrintf("pass\n");

    // 유휴 태스크를 시스템 스레드로 생성하고 셀을 시작
    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0,
                (qword)kIdleTask);
    kStartConsoleShell();
}
