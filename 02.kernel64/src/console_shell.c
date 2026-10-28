#include "console_shell.h"
#include "types.h"
#include "console.h"
#include "keyboard.h"
#include "utility.h"
#include "pit.h"
#include "rtc.h"
#include "assembly_utility.h"
#include "task.h"
#include "synchronization.h"
#include "dynamic_memory.h"
#include "harddisk.h"
#include "file_system.h"
#include "cache_manager.h"
#include "ramdisk.h"


// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
    {"help", "show help", kHelp},
    {"cls", "clear screen", kCls},
    {"totalram", "show total RAM size", kShowTotalRAMSize},
    {"strtod", "string to decimal/hex convert", kStringToDecimalHexTest},
    {"shutdown", "shutdown and reboot OS", kShutdown},
    {"settimer", "set PIT controller counter0, ex)settimer 10(ms) 1(periodic)",
        kSetTimer},
    {"wait", "wait ms using PIT, ex)wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "measure processor speed", kMeasureProcessorSpeed},
    {"date", "show date and time", kShowDateAndTime},
    {"createtask", "create task, ex)createtask 1(type) 10(count)", kCreateTestTask},
    {"changepriority", "change task priority, ex)changepriority 1(ID) 2(priority)",
        kChangeTaskPriority},
    {"tasklist", "show task list", kShowTaskList},
    {"killtask", "end task, ex)killtask 1(ID) or 0xffffffff(all task)", kKillTask},
    {"cpuload", "show processor load", kCPULoad},
    {"testmutex", "test mutex function", kTestMutex},
    {"testthread", "Test Thread And Process Function", kTestThread},
    {"showmatrix", "Show Matrix Screen", kShowMatrix},
    {"testpie", "Test PIE Calculation", kTestPIE},
    {"dynamicmeminfo", "Show Dyanmic Memory Information", kShowDyanmicMemoryInformation},
    {"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
    {"testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
    {"hddinfo", "Show HDD Information", kShowHDDInformation},
    {"readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", kReadSector},
    {"writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", kWriteSector},
    {"mounthdd", "Mount HDD", kMountHDD},
    {"formathdd", "Format HDD", kFormatHDD},
    {"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
    {"createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory},
    {"deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory},
    {"dir", "Show Directory", kShowRootDirectory},
    {"writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile},
    {"readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile},
    {"testfileio", "Test File I/O Function", kTestFileIO},
    {"testperformance", "Test File Read/WritePerformance", kTestPerformance},
    {"flush", "Flush File System Cache", kFlushCache},
};

// 셸의 메인 루프
void kStartConsoleShell(){
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    byte bKey;

    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while(true){
        // 키가 수신될 때까지 대기
        bKey = kGetch();

        // backspace 처리
        if(bKey == KEY_BACKSPACE){
            if(iCommandBufferIndex > 0){
                kPrintf("\b \b");
            iCommandBufferIndex--;
            }
        }
        // 엔터 처리
        else if(bKey == KEY_ENTER){
            kPrintf("\n");

            if(iCommandBufferIndex > 0){
                // 커맨드 실행
                vcCommandBuffer[iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }
            // 프롬프트 출력, 커맨드 버퍼 초기화
            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            iCommandBufferIndex = 0;
        }
        // esc 처리
        else if(bKey == KEY_ESC){}
        // 기타 특수키 처리
        else if(bKey >= 0x80){}

        else{
            if(bKey == KEY_TAB){// tab 처리
                bKey = ' ';
            }
            if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT){
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

// 커맨드 버퍼에 있는 커맨드 실행
void kExecuteCommand(const char* pcCommandBuffer){
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    // 공백으로 구분된 커맨드 추출
    iCommandBufferLength = kStrLen(pcCommandBuffer);
    for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++){
        if(pcCommandBuffer[iSpaceIndex] == ' '){
            break;
        }
    }

    // 커맨드 테이블에 동일한 커맨드 있는지 확인
    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for(i=0; i<iCount; i++){
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        
        if((iCommandLength == iSpaceIndex) &&
            (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)
        ){
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }

    // 리스트에 없으면 에러 출력
    if(i >= iCount){
        kPrintf("'%s' is not found.\n", pcCommandBuffer);
    }
}

// 파라미터 자료구조 초기화
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter){
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

// 공백으로 구분된 파라미터의 내용과 길이를 반환
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter){
    int i, iLength;

    // 파라미터 없으면 종료
    if(pstList->iLength <= pstList->iCurrentPosition){
        return 0;
    }

    // 버퍼의 길이만큼 이동하면서 공백을 검색
    for(i = pstList->iCurrentPosition; i< pstList->iLength; i++){
        if(pstList->pcBuffer[i] == ' '){
            break;
        }
    }

    // 파라미터 복사하고 길이 반환
    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    // 파라미터 위치 업데이트
    pstList->iCurrentPosition += iLength +1;
    return iLength;
}

//===============커맨드 처리 코드========================

// 셸 도움말 출력
static void kHelp(const char* pcCommandBuffer){
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kPrintf("=========================================================\n");
    kPrintf("                      OS Shell Help                      \n");
    kPrintf("=========================================================\n");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    // 가장 긴 커맨드 길이 계산
    for(int i=0; i<iCount; i++){
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength){
            iMaxCommandLength = iLength;
        }
    }

    // 도움말 출력
    for(int i=0; i<iCount; i++){
        kPrintf(gs_vstCommandTable[i].pcCommand);
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf("  - %s\n", gs_vstCommandTable[i].pcHelp);

        // 목록이 많을 경우 나눠서 보여줌
        if((i != 0) && ((i % 20) == 0)){
            kPrintf("press any key to continue... ('q' is exit) : ");
            if(kGetch() == 'q'){
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }
    }
}

// 화면 지우기
static void kCls(const char* pcParameterBuffer){
    // 맨 윗줄은 비우기 (디버깅용)
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
static void kShowTotalRAMSize(const char* pcParameterBuffer){
    kPrintf("total RAM size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
static void kStringToDecimalHexTest(const char* pcParameterBuffer){
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;

    // 파라미터 초기화
    kInitializeParameter(&stList, pcParameterBuffer);

    while(true){
        // 다음 파라미터 얻기
        iLength = kGetNextParameter(&stList, vcParameter);
        // 없으면 종료
        if(iLength == 0){
            break;
        }

        kPrintf("param %d = '%s', length = %d, ", iCount+1, vcParameter, iLength);

        // 0x로 시작하면 16진수, 아니면 10진수
        if(kMemCmp(vcParameter, "0x", 2) == 0){
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX value = %q\n", lValue);
        }
        else{
            lValue = kAToI(vcParameter, 10);
            kPrintf("decimal value = %d\n", lValue);
        }
        iCount++;
    }
}

// pc 재시작
static void kShutdown(const char* pcParameterBuffer){
    kPrintf("System shutdown start...\n");

    // 파일 시스템 캐시에 들어 있는 내용을 하드 디스크로 옮김
    kPrintf("Cache Flush...");
    if(kFlushFileSystemCache() == true)
    {
        kPrintf("Pass\n");
    }
    else
    {
        kPrintf("Fail\n");
    }

    // 키보드 컨트롤러를 통해 pc 재시작
    kPrintf("press any key to reboot PC...");
    kGetch();
    kReboot();
}

// PIT 컨트롤러의 카운터0 설정
static void kSetTimer(const char* pcParameterBuffer){
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    bool bPeriodic;

    // 파라미터 초기화
    kInitializeParameter(&stList, pcParameterBuffer);

    // millisecond 추출
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    lValue = kAToI(vcParameter, 10);

    // Periodic 추출
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    bPeriodic = kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("time = %dms, periodic = %d change complete\n", lValue, bPeriodic);
}

// PIT 컨트롤러를 직접 사용하여 ms 동안 대기
static void kWaitUsingPIT(const char* pcParameterBuffer){
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;

    // 파라미터 초기화
    kInitializeParameter(&stList, pcParameterBuffer);
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)wait 100(ms)\n");
        return;
    }

    lMillisecond = kAToI(pcParameterBuffer, 10);
    kPrintf("%dms sleep start...\n", lMillisecond);

    // 인터럽트 비활성화하고 PIT 컨트롤러를 통해 직접 시간 측정
    kDisableInterrupt();
    for(int i=0; i< lMillisecond/30; i++){
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();
    kPrintf("%dms sleep complete\n", lMillisecond);

    // 타이머 복원
    kInitializePIT(MSTOCOUNT(1), true);
}

// 타임 스탬프 카운터를 읽음
static void kReadTimeStampCounter(const char* pcParameterBuffer){
    qword qwTSC;

    qwTSC = kReadTSC();
    kPrintf("time stamp counter = %q\n", qwTSC);
}

// 프로세서의 속도를 측정
static void kMeasureProcessorSpeed(const char* pcParameterBuffer){
    qword qwLastTSC, qwTotalTSC = 0;

    kPrintf("now measuring");

    // 10초 동안 변화한 타임 스탬프 카운터를 이용하여 프로세서의 속도를 간접적으로 측정
    kDisableInterrupt();
    for(int i=0; i<200; i++){
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf(".");
    }
    // 타이머 복원
    kInitializePIT(MSTOCOUNT(1), true);
    kEnableInterrupt();

    kPrintf("\nCPU speed = %dMHz\n", qwTotalTSC / 100 / 1000 / 1000);
}

// RTC 컨트롤러에 저장된 일자 및 시간 정보를 표시
static void kShowDateAndTime(const char* pcParameterBuffer){
    byte bSecond, bMinute, bHour;
    byte bDayOfWeek, bDayOfMonth, bMonth;
    word wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("date: %d/%d/%d %s, ",
        wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("time: %d:%d:%d\n", bHour, bMinute, bSecond);
}





// 태스크 1: 화면 테두리를 돌면서 문자를 출력
static void kTestTask1(){
    byte bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

    // 화면 네 귀퉁이를 돌면서 문자 출력
    for(int j=0; j<20000; j++){
        switch(i){
        case 0:
            iX++;
            if(iX >= (CONSOLE_WIDTH - iMargin)){
                i = 1;
            }
            break;

        case 1:
            iY++;
            if(iY >= (CONSOLE_HEIGHT - iMargin)){
                i = 2;
            }
            break;
        
        case 2:
            iX--;
            if(iX < iMargin){
                i = 3;
            }
            break;

        case 3:
            iY--;
            if(iY < iMargin){
                i = 0;
            }
            break;
        }
        // 문자 및 색깔 지정
        pstScreen[iY * CONSOLE_WIDTH + iX].bCharacter = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0f;
        bData++;

        // 다른 태스크로 전환
        kSchedule();
    }

    kExitTask();
}

// 태스크 2: 자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
static void kTestTask2(){
    int i = 0, iOffset;
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(true){
        // 회전하는 바람개비를 표시
        pstScreen[iOffset].bCharacter = vcData[i%4];
        // 색깔 지정
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        // 다른 태스크로 전환
        kSchedule();
    }
}

// 태스크 생성해서 멀티태스킹 수행
static void kCreateTestTask(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;

    // 파라미터를 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    switch(kAToI(vcType, 10)){
    // 타입 1 태스크 생성
    case 1:
        for(i=0; i<kAToI(vcCount, 10); i++){
            if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0,
                           (qword)kTestTask1) == null){
                break;
            }
        }
        kPrintf("task1 %d created\n", i);
        break;

    // 타입 2 태스크 생성
    case 2:
    default:
        for(i=0; i<kAToI(vcCount, 10); i++){
            if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0,
                           (qword)kTestTask2) == null){
                break;
            }
        }

        kPrintf("task2 %d created\n", i);
        break;
    }
}

// 태스크의 우선순위를 변경
static void kChangeTaskPriority(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcID[30];
    char vcPriority[30];
    qword qwID;
    byte bPriority;

    // 파라미터 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcPriority);

    // 태스크의 우선순위를 변경
    if(kMemCmp(vcID, "0x", 2) == 0){
        qwID = kAToI(vcID + 2, 16);
    }
    else{
        qwID = kAToI(vcID, 10);
    }

    bPriority = kAToI(vcPriority, 10);

    kPrintf("change task priority ID [0x%q] priority[%d] ", qwID, bPriority);
    if(kChangePriority(qwID, bPriority) == true){
        kPrintf("success\n");
    }
    else{
        kPrintf("fail\n");
    }
}

// 현재 생성된 모든 태스크의 정보를 출력
static void kShowTaskList(const char* pcParameterBuffer){
    TCB* pstTCB;
    int iCount = 0;

    kPrintf("=========== task total count [%d] ===========\n", kGetTaskCount());
    for(int i=0; i<TASK_MAXCOUNT; i++){
        // TCB를 구해서 TCB가 사용 중이면 ID를 출력
        pstTCB = kGetTCBInTCBPool(i);
        if((pstTCB->stLink.qwID >> 32) != 0){
            // 태스크가 10개 출력될 때마다, 계속 태스크 정보를 표시할지 여부를 확인
            if((iCount != 0) && (iCount % 10) == 0){
                kPrintf("press any key to continue... ('q' is exit) : ");
                if(kGetch() == 'q'){
                    kPrintf("\n");
                    break;
                }
                kPrintf("\n");
            }
            kPrintf("[%d] Task ID[0x%q], Priority[%d], Flags[0x%q], Thread[%d]\n", 1 + iCount++,
                    pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwFlags,
                    kGetListCount(&(pstTCB->stChildThreadList)));
            kPrintf("    Parent PID[0x%q], Memory Address[0x%q], Size[0x%q]\n",
                    pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
        }
    }
}

// 태스크 종료
static void kKillTask(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcID[30];
    qword qwID;
    TCB* pstTCB;

    // 파라미터를 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);

    // 태스크를 종료
    if(kMemCmp(vcID, "0x", 2) == 0){
        qwID = kAToI(vcID + 2, 16);
    }
    else{
        qwID = kAToI(vcID, 10);
    }

    // 특정 ID만 종료하는 경우
    if(qwID != 0xffffffff){
        pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
        qwID = pstTCB->stLink.qwID;

        //시스템 테스트는 제외
        if(((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)){
            kPrintf("kill task ID [0x%q] ", qwID);
            if(kEndTask(qwID) == true){
                kPrintf("success\n");
            }
            else{
                kPrintf("fail\n");
            }
        }
        else{
            kPrintf("task does not exists or task is system task\n");
        }
    }

    // 콘솔 셸과 유휴 태스크를 제외하고 모든 태스크 종료
    else{
        for(int i=0; i<TASK_MAXCOUNT; i++){
            pstTCB = kGetTCBInTCBPool(i);
            qwID = pstTCB->stLink.qwID;

            // 시스템 태스크는 삭제 목록에서 제외
            if(((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)){
                kPrintf("kill task ID [0x%q]", qwID);
                if(kEndTask(qwID) == true){
                    kPrintf("success\n");
                }
                else{
                    kPrintf("fail\n");
                }
            }
        }
    }
}

// 프로세서의 사용률 표시
static void kCPULoad(const char* pcParameterBuffer){
    kPrintf("processor load: %d%%\n", kGetProcessorLoad());
}

// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_stMutex;
static volatile qword gs_qwAdder;

// 뮤텍스를 테스트하는 태스크
static void kPrintNumberTask(){
    qword qwTickCount;

    // 50ms 정도 대기하여 콘솔 셸이 출력하는 메시지와 겹치지 않도록 함
    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 50){
        kSchedule();
    }

    // 루프를 돌며넛 숫자를 출력
    for(int i=0; i<5; i++){
        kLock(&(gs_stMutex));
        kPrintf("task ID [0x%q] value[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);

        gs_qwAdder += 1;
        kUnlock(&(gs_stMutex));

        // 프로세서 소모를 늘리려고 추가한 코드
        for(volatile int j=0; j<30000; j++);
    }

    // 모든 태스크가 종료할 때까지 1초 정도 대기
    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 1000){
        kSchedule();
    }

    // 태스크 종료
    kExitTask();
}

// 뮤텍스를 테스트하는 태스크 생성
static void kTestMutex(const char* pcParameterBuffer){
    gs_qwAdder = 1;
    int i;

    // 뮤텍스 초기화
    kInitializeMutex(&gs_stMutex);

    for(i=0; i<3; i++){
        // 뮤텍스를 테스트하는 테스크를 3개 생성
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)kPrintNumberTask);
    }
    kPrintf("wait until %d task end...\n", i);
    kGetch();
}

// 태스크 2를 자신의 스레드로 생성하는 테스크
static void kCreateThreadTask(){
    for(int i=0; i<3; i++){
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)kTestTask2);
    }
    while(true){
        kSleep(1);
    }
}

// 스레드를 태스트하는 테스크 생성
static void kTestThread(const char* pcParameterBuffer){
    TCB* pstProcess;

    pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xeeeeeeee, 0x1000,
                             (qword)kCreateThreadTask);
    if(pstProcess != null){
        kPrintf("process [0x%q] create success\n", pstProcess->stLink.qwID);
    }
    else{
        kPrintf("process create fail\n");
    }
}

// 난수를 발생시키기 위한 변수
static volatile qword gs_qwRandomValue = 0;

// 임의의 난수를 반환
qword kRandom(){
    gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
    return gs_qwRandomValue;
}

// 철자를 흘러내리게 하는 스레드
static void kDropCharactorThread(){
    int iX;
    char vcText[2] = {0,};

    iX = kRandom() % CONSOLE_WIDTH;

    while(true){
        // 잠시 대기함
        kSleep(kRandom() % 20);

        if((kRandom() % 20) < 16){
            vcText[0] = ' ';
            for(int i=0; i<CONSOLE_HEIGHT-1; i++){
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        }
        else{
            for(int i=0; i<CONSOLE_HEIGHT-1; i++){
                vcText[0] = i + kRandom();
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        }
    }
}

// 스레드를 생성하여 매트릭스 화면처럼 보여주는 프로세스
static void kMatrixProcess(){
    int i;
    for(i=0; i<300; i++){
        if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
                        (qword)kDropCharactorThread) == null){
            
            break;
        }

        kSleep(kRandom() % 5 + 5);
    }
    kPrintf("%d thread is created\n", i);

    // 키가 입력되면 프로세스 종료
    kGetch();
}

//매트릭스 화면을 보여줌
static void kShowMatrix(const char* pcParameterBuffer){
    TCB* pstProcess;

    pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xe00000, 0xe00000,
                             (qword)kMatrixProcess);
    if(pstProcess != null){
        kPrintf("matrix process [0x%q] create success\n");

        // 태스크 종료될 때까지 대기
        while((pstProcess->stLink.qwID >> 32) != 0){
            kSleep(100);
        }
    }
    else{
        kPrintf("matrix process create fail\n");
    }
}

// FPU를 테스트하는 태스크
static void kFPUTestTask(void){
    double dValue1;
    double dValue2;
    TCB* pstRunningTask;
    qword qwCount = 0;
    qword qwRandomValue;
    int iOffset;
    char vcData[4] = {'-', '\\', '|', '/'};
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;

    pstRunningTask = kGetRunningTask();

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset & (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    // 루프를 무한히 반복하면서 동일한 계산을 수행
    while(true){
        dValue1 = 1;
        dValue2 = 1;

        // 테스트를 위해 동일한 계산을 2번 반복해서 실행
        for(int i=0; i<10; i++){
            qwRandomValue = kRandom();
            dValue1 *= (double)qwRandomValue;
            dValue2 *= (double)qwRandomValue;

            kSleep(1);

            qwRandomValue = kRandom();
            dValue1 /= (double)qwRandomValue;
            dValue2 /= (double)qwRandomValue;
        }

        if (dValue1 != dValue2){
            kPrintf("value is not same [%f] != [%f]\n", dValue1, dValue2);
            break;
        }
        qwCount++;

        // 회전하는 바람개비를 표시
        pstScreen[iOffset].bCharacter = vcData[qwCount % 4];

        // 색깔 지정
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
    }
}

// 원주율(PIE)를 계산
static void kTestPIE(const char* pcParameterBuffer){
    double dResult;

    kPrintf("PIE calculation test\n");
    kPrintf("Result: 355 / 113 = ");
    dResult = (double) 355 / 113;
    kPrintf("%d.%d%d\n", (qword)dResult, ((qword)(dResult * 10) % 10),
            ((qword)(dResult*100) % 10));
    // 실수를 계산하는 태스크를 생성
    for(int i = 0; i<100; i++){
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)kFPUTestTask);
    }
}

// 동적 메모리 정보를 표시
static void kShowDyanmicMemoryInformation(const char* pcParameterBuffer){
    qword qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

    kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);
    
    kPrintf("============ Dynamic Memory Information ============\n");
    kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
    kPrintf("Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize,
            qwTotalSize / 1024 / 1024);
    kPrintf("Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize,
            qwMetaSize / 1024);
    kPrintf("Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);
}

// 모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
static void kTestSequentialAllocation(const char* pcParameterBuffer){
    DYNAMICMEMORY* pstMemory;
    long i, j, k;
    volatile qword* pqwBuffer;

    kPrintf("============ Dynamic Memory Test ============\n");
    pstMemory = kGetDynamicMemoryManager();

    for(i=0; i<pstMemory->iMaxLevelCount; i++){
        kPrintf("Block List [%d] Test Start\n", i);
        kPrintf("Allocation And Compare: ");

        // 모든 블록을 할당 받아서 값을 채운 후 검사
        for(j=0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){
            pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
            if(pqwBuffer == null){
                kPrintf("\nAllocation Fail\n");
                return;
            }

            // 값을 채운 후 다시 검사
            for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i) / 8; k++){
                pqwBuffer[k] = k;
            }

            for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i) / 8; k++){
                if(pqwBuffer[k] != k){
                    kPrintf("\nCompare Fail\n");
                    return;
                }
            }
            // 진행 과정을 .으로 표시
            kPrintf(".");
        }

        kPrintf("\nFree");
        // 할당 받은 블록을 모두 반환
        for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){
            if(kFreeMemory((void*)(pstMemory->qwStartAddress +
                (DYNAMICMEMORY_MIN_SIZE << i) * j)) == false)
            {
                kPrintf("\nFree Fail\n");
                return;
            }
            // 진행 과정을 .으로 표시
            kPrintf(".");
        }
        kPrintf("\n");
    }
    kPrintf("Test Complete\n");
}

// 임의로 메모리를 할당하고 해제하는 것을 반복하는 테스크
static void kRandomAllocationTask(){
    TCB* pstTask;
    qword qwMemorySize;
    char vcBuffer[200];
    volatile byte* pbAllocationBuffer;
    int i, j;
    int iY;

    pstTask = kGetRunningTask();
    iY = (pstTask->stLink.qwID) % 15 + 9;
    
    for(j=0; j<10; j++){
        // 1KB ~ 32MB 까지 할당하도록 함
        do{
            qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
            pbAllocationBuffer = kAllocateMemory(qwMemorySize);

            // 만일 버퍼를 할당 받지 못하면 다른 태스크가 메모리를 사용하고
            // 있을 수 있으므로 잠시 대기한 후 다시 시도
            if(pbAllocationBuffer == 0){
                kSleep(1);
            }
        }while(pbAllocationBuffer == 0);

        kSprintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success",
                pbAllocationBuffer, qwMemorySize);
        // 자신의 ID를 Y좌표로 하여 데이터를 출력
        kPrintStringXY(20 ,iY, vcBuffer);
        kSleep(200);

        // 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움
        kSprintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ",
                pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for(i=0; i<qwMemorySize/2; i++){
            pbAllocationBuffer[i] = kRandom() & 0xff;
            pbAllocationBuffer[i + (qwMemorySize / 2)] = pbAllocationBuffer[i];
        }
        kSleep(200);

        // 채운 데이터가 정상적인지 다시 확인
        kSprintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ",
                pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for(i=0; i<qwMemorySize / 2; i++){
            if(pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize / 2)]){
                kPrintf("Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID);
                kExitTask();
            }
        }
        kFreeMemory((void*)pbAllocationBuffer);
        kSleep(200);
    }

    kExitTask();
}

// 태스크를 여러 개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
static void kTestRandomAllocation(const char* pcParameterBuffer){
    int i;

    for(i=0; i<1000; i++){
        kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (qword)kRandomAllocationTask);
    }
}

// 하드 디스크의 정보를 표시
static void kShowHDDInformation(const char* pcParameterBuffer){
    HDDINFORMATION stHDD;
    char vcBuffer[100];

    // 하드 디스크의 정보를 읽음
    if(kReadHDDInformation(true, false, &stHDD) == false){
        kPrintf("HDD information read fail\n");
        return;
    }

    kPrintf("============ Primary Slave HDD Information ============\n");

    // 모델 번호 출력
    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber)] = '\0';
    kPrintf("Model Number:\t %s\n", vcBuffer);

    // 시리얼 번호 출력
    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber)] = '\0';
    kPrintf("Serial Number:\t %s\n", vcBuffer);

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

    // 총 섹터 수 출력
    kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors,
            stHDD.dwTotalSectors / 2 / 1024);
}

// 하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 읽음
static void kReadSector(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcLBA[50], vcSectorCount[50];
    dword dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    byte bData;
    bool bExit = false;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    if((kGetNextParameter(&stList, vcLBA) == 0) ||
        (kGetNextParameter(&stList, vcSectorCount) == 0))
    {
        kPrintf("ex) readsector 0(LBA) 10(count)\n");
        return;
    }
    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);

    if(iSectorCount <= 0 || iSectorCount > 256){
        kPrintf("Sector count must be between 1 and 256\n");
        return;
    }

    // 섹터 수만큼 메모리를 할당 받아 읽기 수행
    pcBuffer = kAllocateMemory(iSectorCount * 512);
    if(pcBuffer == null){
        kPrintf("Memory allocation fail\n");
        return;
    }

    if(kReadHDDSector(true, false, dwLBA, iSectorCount, pcBuffer) == iSectorCount){
        kPrintf("LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount);
        // 데이터 버퍼의 내용을 출력
        for(j=0; j<iSectorCount; j++){
            for(i=0; i<512; i++){
                if(!((j==0) && (i==0)) && ((i%256)==0)){
                    kPrintf("\nPress any key to continue... ('q' is exit) : ");
                    if(kGetch() == 'q'){
                        bExit = true;
                        break;
                    }
                }
                if((i%16) == 0){
                    kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
                bData = pcBuffer[j* 512 + i] & 0xff;
                if(bData < 16){
                    kPrintf("0");
                }
                kPrintf("%X ", bData);
            }

            if(bExit == true){
                break;
            }
        }
        kPrintf("\n");
    }
    else{
        kPrintf("Read Fail\n");
    }
    kFreeMemory(pcBuffer);
}

// 하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 씀
static void kWriteSector(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcLBA[50], vcSectorCount[50];
    dword dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    bool bExit = false;
    byte bData;
    static dword s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    if((kGetNextParameter(&stList, vcLBA) == 0) ||
        (kGetNextParameter(&stList, vcSectorCount) == 0))
    {
        kPrintf("ex) writesector 0(LBA) 10(count)\n");
        return;
    }
    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);
    if(iSectorCount <= 0 || iSectorCount > 256){
        kPrintf("Sector count must be between 1 and 256\n");
        return;
    }

    s_dwWriteCount++;

    // 버퍼를 할당 받아 데이터를 채움. 
    // 패턴은 4 바이트의 LBA 어드레스와 4 바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory(iSectorCount * 512);
    if(pcBuffer == null){
        kPrintf("Memory allocation fail\n");
        return;
    }
    for(j=0; j<iSectorCount; j++){
        for(i=0; i<512; i+=8){
            *(dword*)&(pcBuffer[j * 512 + i]) = dwLBA + j;
            *(dword*)&(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
        }
    }

    // 쓰기 수행
    if(kWriteHDDSector(true, false, dwLBA, iSectorCount, pcBuffer) != iSectorCount){
        kPrintf("Write Fail\n");
        kFreeMemory(pcBuffer);
        return;
    }
    kPrintf("LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount);

    // 데이터 버퍼의 내용을 출력
    for(j=0; j<iSectorCount; j++){
        for(i=0 ; i<512; i++){
            if(!((j==0) && (i==0)) && ((i%256) == 0)){
                kPrintf("\nPress any key to continue... ('q' is exit) : ");
                if(kGetch() == 'q'){
                    bExit = true;
                    break;
                }
            }

            if((i % 16) == 0){
                kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
            }

            // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
            bData = pcBuffer[j * 512 + i] & 0xff;
            if(bData < 16){
                kPrintf("0");
            }
            kPrintf("%X ", bData);
        }

        if(bExit == true){
            break;
        }
    }
    kPrintf("\n");
    kFreeMemory(pcBuffer);
}

// 하드 디스크를 연결
static void kMountHDD(const char* pcParameterBuffer){
    if(kMount() == false){
        kPrintf("HDD Mount Fail\n");
        return;
    }
    kPrintf("HDD Mount Success\n");
}

// 하드 디스크에 파일 시스템을 생성(포맷)
static void kFormatHDD(const char* pcParameterBuffer){
    if(kFormat() == false){
        kPrintf("HDD Format Fail\n");
        return;
    }
    kPrintf("HDD Format Success\n");
}

// 파일 시스템 정보를 표시
static void kShowFileSystemInformation(const char* pcParameterBuffer){
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation(&stManager);

    kPrintf("================== File System Information ==================\n");
    kPrintf("Mouted:\t\t\t\t\t %d\n", stManager.bMounted );
    kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
    kPrintf("Cluster Link Table Start Address:\t %d Sector\n", 
            stManager.dwClusterLinkAreaStartAddress);
    kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
    kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
    kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

// 루트 디렉터리에 빈 파일을 생성
static void kCreateFileInRootDirectory(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    FILE* pstFile;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)){
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    pstFile = fopen(vcFileName, "w");
    if(pstFile == null){
        kPrintf("file create fail\n");
        return;
    }
    fclose(pstFile);
    kPrintf("file create success\n");
}

// 루트 디렉터리에서 파일을 삭제
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)){
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    if(remove(vcFileName) != 0){
        kPrintf("File Not Found or File Opened\n");
        return;
    }
    
    kPrintf("File Delete Success\n");
}

// 루트 디렉터리의 파일 목록을 표시
static void kShowRootDirectory(const char* pcParameterBuffer){
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[400];
    char vcTempValue[50];
    dword dwTotalByte;
    dword dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;

    // 파일 시스템 정보를 얻음
    kGetFileSystemInformation(&stManager);

    // 루트 디렉터리를 엶
    pstDirectory = opendir("/");
    if(pstDirectory == null){
        kPrintf("Root Directory Open Fail\n");
        return;
    }

    // 먼저 루프를 돌면서 디렉터리에 있는 파일의 개수와 전체 파일이 사용한 크기를 계산
    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;
    while(1){
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == null )
        {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // 실제로 사용된 클러스터의 개수를 계산
        if( pstEntry->dwFileSize == 0 )
        {
            // 크기가 0이라도 클러스터 1개는 할당되어 있음
            dwUsedClusterCount++;
        }
        else
        {
            // 클러스터 개수를 올림하여 더함
            dwUsedClusterCount += ( pstEntry->dwFileSize + 
                ( FILESYSTEM_CLUSTERSIZE - 1 ) ) / FILESYSTEM_CLUSTERSIZE;
        }
    }

    // 실제 파일의 내용을 표시하는 루프
    rewinddir( pstDirectory );
    iCount = 0;
    while(1){
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == null )
        {
            break;
        }

        // 전부 공백으로 초기화 한 후 각 위치에 값을 대입
        kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
        vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';
        
        // 파일 이름 삽입
        kMemCpy( vcBuffer, pstEntry->d_name, 
                 kStrLen( pstEntry->d_name ) );

        // 파일 길이 삽입
        kSprintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
        kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue ) );

        // 파일의 시작 클러스터 삽입
        kSprintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
        kMemCpy( vcBuffer + 55, vcTempValue, kStrLen( vcTempValue ) + 1 );
        kPrintf( "    %s\n", vcBuffer );

        if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetch() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
        }
        iCount++;
    }

    // 총 파일의 개수와 파일의 총 크기를 출력
    kPrintf( "\t\tTotal File Count: %d\n", iTotalCount );
    kPrintf( "\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte, 
             dwUsedClusterCount );
    
    // 남은 클러스터 수를 이용해서 여유 공간을 출력
    kPrintf( "\t\tFree Space: %d KByte (%d Cluster)\n", 
             ( stManager.dwTotalClusterCount - dwUsedClusterCount ) * 
             FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - 
             dwUsedClusterCount );
    
    // 디렉터리를 닫음
    closedir( pstDirectory );

}

// 파일을 생성하여 키보드로 입력된 데이터를 씀
static void kWriteDataToFile(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    byte bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "w" );
    if( fp == null )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 엔터 키가 연속으로 3번 눌러질 때까지 내용을 파일에 씀
    iEnterCount = 0;
    while( 1 )
    {
        bKey = kGetch();
        // 엔터 키이면 연속 3번 눌러졌는가 확인하여 루프를 빠져 나감
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            if( iEnterCount >= 3 )
            {
                break;
            }
        }
        // 엔터 키가 아니라면 엔터 키 입력 횟수를 초기화
        else
        {
            iEnterCount = 0;
        }
        
        kPrintf( "%c", bKey );
        if( fwrite( &bKey, 1, 1, fp ) != 1 )
        {
            kPrintf( "File Wirte Fail\n" );
            break;
        }
    }
    
    kPrintf( "File Create Success\n" );
    fclose( fp );
}

// 파일을 열어서 데이터를 읽음
static void kReadDataFromFile(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    byte bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "r" );
    if( fp == null )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 파일의 끝까지 출력하는 것을 반복
    iEnterCount = 0;
    while( 1 )
    {
        dword r = fread( &bKey, 1, 1, fp );
        if( r != 1 )
        {
            break;
        }
        kPrintf( "%c", bKey );
        
        // 만약 엔터 키이면 엔터 키 횟수를 증가시키고 20라인까지 출력했다면 
        // 더 출력할지 여부를 물어봄
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            
            if( ( iEnterCount != 0 ) && ( ( iEnterCount % 20 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetch() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
                iEnterCount = 0;
            }
        }
    }
    fclose( fp );
}

// 파일 I/O에 관련된 기능을 테스트
static void kTestFileIO(const char* pcParameterBuffer){
    FILE* pstFile;
    byte* pbBuffer;
    int i;
    int j;
    dword dwRandomOffset;
    dword dwByteCount;
    byte vbTempBuffer[ 1024 ];
    dword dwMaxFileSize;
    
    kPrintf( "================== File I/O Function Test ==================\n" );
    
    // 4Mbyte의 버퍼 할당
    dwMaxFileSize = 4 * 1024 * 1024;
    pbBuffer = kAllocateMemory( dwMaxFileSize );
    if( pbBuffer == null )
    {
        kPrintf( "Memory Allocation Fail\n" );
        return ;
    }
    // 테스트용 파일을 삭제
    remove( "testfileio.bin" );

    //==========================================================================
    // 파일 열기 테스트
    //==========================================================================
    kPrintf( "1. File Open Fail Test..." );
    // r 옵션은 파일을 생성하지 않으므로, 테스트 파일이 없는 경우 NULL이 되어야 함
    pstFile = fopen( "testfileio.bin", "r" );
    if( pstFile == null )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
        fclose( pstFile );
    }
    
    //==========================================================================
    // 파일 생성 테스트
    //==========================================================================
    kPrintf( "2. File Create Test..." );
    // w 옵션은 파일을 생성하므로, 정상적으로 핸들이 반환되어야함
    pstFile = fopen( "testfileio.bin", "w" );
    if( pstFile != null )
    {
        kPrintf( "[Pass]\n" );
        kPrintf( "2   File Handle [0x%Q]\n", pstFile );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // 순차적인 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "3. Sequential Write Test(Cluster Size)..." );
    // 열린 핸들을 가지고 쓰기 수행
    for( i = 0 ; i < 100 ; i++ )
    {
        kMemSet( pbBuffer, i, FILESYSTEM_CLUSTERSIZE );
        if( fwrite( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "3   %d Cluster Error\n", i );
            break;
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "4. Sequential Read And Verify Test(Cluster Size)..." );
    // 파일의 처음으로 이동
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END );
    
    // 열린 핸들을 가지고 읽기 수행 후, 데이터 검증
    for( i = 0 ; i < 100 ; i++ )
    {
        // 파일을 읽음
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        // 데이터 검사
        for( j = 0 ; j < FILESYSTEM_CLUSTERSIZE ; j++ )
        {
            if( pbBuffer[ j ] != ( byte ) i )
            {
                kPrintf( "[Fail]\n" );
                kPrintf( "4   %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[ j ], 
                         ( byte ) i );
                break;
            }
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // 임의의 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "5. Random Write Test...\n" );
    
    // 버퍼를 모두 0으로 채움
    kMemSet( pbBuffer, 0, dwMaxFileSize );
    // 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
    // 파일의 내용을 읽어서 버퍼로 복사
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR );
    fread( pbBuffer, 1, dwMaxFileSize, pstFile );
    
    // 임의의 위치로 옮기면서 데이터를 파일과 버퍼에 동시에 씀
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( dwMaxFileSize - dwByteCount );
        
        kPrintf( "5   [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );

        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        kMemSet( vbTempBuffer, i, dwByteCount );
              
        // 데이터를 씀
        if( fwrite( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
        
        kMemSet( pbBuffer + dwRandomOffset, i, dwByteCount );
    }
    
    // 맨 마지막으로 이동하여 1바이트를 써서 파일의 크기를 4Mbyte로 만듦
    fseek( pstFile, dwMaxFileSize - 1, SEEK_SET );
    fwrite( &i, 1, 1, pstFile );
    pbBuffer[ dwMaxFileSize - 1 ] = ( byte ) i;

    //==========================================================================
    // 임의의 영역 읽기 테스트
    //==========================================================================
    kPrintf( "6. Random Read And Verify Test...\n" );
    // 임의의 위치로 옮기면서 파일에서 데이터를 읽어 버퍼의 내용과 비교
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( ( dwMaxFileSize ) - dwByteCount );

        kPrintf( "6   [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );
        
        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        
        // 데이터 읽음
        if( fread( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Read Fail\n", dwRandomOffset ); 
            break;
        }
        
        // 버퍼와 비교
        if( kMemCmp( pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount ) 
                != 0 )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "6   Compare Fail\n", dwRandomOffset ); 
            break;
        }
        
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // 다시 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "7. Sequential Write, Read And Verify Test(1024 Byte)...\n" );
    // 파일의 처음으로 이동
    fseek( pstFile, 0, SEEK_SET );
    
    // 열린 핸들을 가지고 쓰기 수행. 앞부분에서 2Mbyte만 씀
    for( i = 0 ; i < ( 2 * 1024 * 1024 / 1024 ) ; i++ )
    {
        kPrintf( "7   [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024 );

        // 1024 바이트씩 파일을 씀
        if( fwrite( pbBuffer + ( i * 1024 ), 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }

    // 파일의 처음으로 이동
    fseek( pstFile, -dwMaxFileSize, SEEK_SET );
    
    // 열린 핸들을 가지고 읽기 수행 후 데이터 검증. Random Write로 데이터가 잘못 
    // 저장될 수 있으므로 검증은 4Mbyte 전체를 대상으로 함
    for( i = 0 ; i < ( dwMaxFileSize / 1024 )  ; i++ )
    {
        // 데이터 검사
        kPrintf( "7   [%d] Offset [%d] Byte [%d] Read And Verify...", i, 
                i * 1024, 1024 );
        
        // 1024 바이트씩 파일을 읽음
        if( fread( vbTempBuffer, 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        if( kMemCmp( pbBuffer + ( i * 1024 ), vbTempBuffer, 1024 ) != 0 )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }
        
    //==========================================================================
    // 파일 삭제 실패 테스트
    //==========================================================================
    kPrintf( "8. File Delete Fail Test..." );
    // 파일이 열려있는 상태이므로 파일을 지우려고 하면 실패해야 함
    if( remove( "testfileio.bin" ) != 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // 파일 닫기 테스트
    //==========================================================================
    kPrintf( "9. File Close Test..." );
    // 파일이 정상적으로 닫혀야 함
    if( fclose( pstFile ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // 파일 삭제 테스트
    //==========================================================================
    kPrintf( "10. File Delete Test..." );
    // 파일이 닫혔으므로 정상적으로 지워져야 함 
    if( remove( "testfileio.bin" ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    // 메모리를 해제
    kFreeMemory( pbBuffer );
}

// 파일을 읽고 쓰는 속도를 측정
static void kTestPerformance(const char* pcParameterBuffer)
{
    FILE* pstFile;
    dword dwClusterTestFileSize;
    dword dwOneByteTestFileSize;
    qword qwLastTickCount;
    dword i;
    byte* pbBuffer;

    // 클러스터는 1MB 까지 파일을 테스트
    dwClusterTestFileSize = 1024 * 1024;
    // 1바이트씩 읽고 쓰는 테스트는 시간이 많이 걸리므로 16KB만 테스트
    dwOneByteTestFileSize = 16 * 1024;

    // 테스트용 버퍼 메모리 할당
    pbBuffer = kAllocateMemory(dwClusterTestFileSize);
    if(pbBuffer == null)
    {
        kPrintf("Memory Allocation Fail\n");
        return;
    }

    // 버퍼를 초기화
    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

    kPrintf("================== File I/O Performance Test ==================\n");

    //==========================================================================
    // 클러스터 단위로 파일을 순차적으로 쓰는 테스트
    //==========================================================================
    kPrintf("1.Sequential Read/Write Test(Cluster Size)\n");

    // 기존의 테스트 파일을 제거하고 새로 만듦
    remove("performance.txt");
    pstFile = fopen("performance.txt", "w");
    if(pstFile == null)
    {
        kPrintf("File Open Fail\n");
        kFreeMemory(pbBuffer);
        return;
    }

    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 쓰는 테스트
    for(i=0; i<(dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++)
    {
        if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) !=
            FILESYSTEM_CLUSTERSIZE)
        {
            kPrintf("Write Fail\n");
            // 파일을 닫고 메모리를 해제함
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    // 시간 출력
    kPrintf("   Sequential Write(Cluster Size): %d ms\n", kGetTickCount() - 
             qwLastTickCount);

    //==========================================================================
    // 클러스터 단위로 파일을 순차적으로 읽는 테스트
    //==========================================================================
    // 파일의 처음으로 이동
    fseek( pstFile, 0, SEEK_SET );

    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 읽는 테스트
    for( i = 0 ; i < ( dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE ) ; i++ )
    {
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) != 
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "Read Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Read(Cluster Size): %d ms\n", kGetTickCount() - 
             qwLastTickCount );
             
    //==========================================================================
    // 1 바이트 단위로 파일을 순차적으로 쓰는 테스트
    //==========================================================================
    kPrintf( "2.Sequential Read/Write Test(1 Byte)\n" );
    
    // 기존의 테스트 파일을 제거하고 새로 만듦
    remove( "performance.txt" );
    pstFile = fopen( "performance.txt", "w" );
    if( pstFile == null )
    {
        kPrintf( "File Open Fail\n" );
        kFreeMemory( pbBuffer );
        return ;
    }
    
    qwLastTickCount = kGetTickCount();
    // 1 바이트 단위로 쓰는 테스트
    for( i = 0 ; i < dwOneByteTestFileSize ; i++ )
    {
        if( fwrite( pbBuffer, 1, 1, pstFile ) != 1 )
        {
            kPrintf( "Write Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Write(1 Byte): %d ms\n", kGetTickCount() - 
             qwLastTickCount );

    //==========================================================================
    // 1 바이트 단위로 파일을 순차적으로 읽는 테스트
    //==========================================================================
    // 파일의 처음으로 이동
    fseek( pstFile, 0, SEEK_SET );
    
    qwLastTickCount = kGetTickCount();
    // 1 바이트 단위로 읽는 테스트
    for( i = 0 ; i < dwOneByteTestFileSize ; i++ )
    {
        if( fread( pbBuffer, 1, 1, pstFile ) != 1 )
        {
            kPrintf( "Read Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Read(1 Byte): %d ms\n", kGetTickCount() - 
             qwLastTickCount );
    
    // 파일을 닫고 메모리를 해제함
    fclose( pstFile );
    kFreeMemory( pbBuffer );
}

// 파일 시스템의 캐시 버퍼에 있는 데이터를 모두 하드 디스크에 씀
static void kFlushCache(const char* pcParameterBuffer)
{
    qword qwTickCount;

    qwTickCount = kGetTickCount();
    kPrintf("Cache Flush...");
    if(kFlushFileSystemCache() == true)
    {
        kPrintf("pass\n");
    }
    else
    {
        kPrintf("fain\n");
    }
    kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}