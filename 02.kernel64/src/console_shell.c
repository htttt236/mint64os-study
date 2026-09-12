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
    { "testpie", "Test PIE Calculation", kTestPIE },
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