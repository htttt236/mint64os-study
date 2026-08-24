#include "console_shell.h"
#include "types.h"
#include "console.h"
#include "keyboard.h"
#include "utility.h"
#include "pit.h"
#include "rtc.h"
#include "assembly_utility.h"
#include "task.h"


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
    {"createtask", "create task", kCreateTestTask}
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
void kHelp(const char* pcCommandBuffer){
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
void kCls(const char* pcParameterBuffer){
    // 맨 윗줄은 비우기 (디버깅용)
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
void kShowTotalRAMSize(const char* pcParameterBuffer){
    kPrintf("total RAM size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
void kStringToDecimalHexTest(const char* pcParameterBuffer){
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
void kShutdown(const char* pcParameterBuffer){
    kPrintf("System shutdown start...\n");

    // 키보드 컨트롤러를 통해 pc 재시작
    kPrintf("press any key to reboot PC...");
    kGetch();
    kReboot();
}

// PIT 컨트롤러의 카운터0 설정
void kSetTimer(const char* pcParameterBuffer){
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
void kWaitUsingPIT(const char* pcParameterBuffer){
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
void kReadTimeStampCounter(const char* pcParameterBuffer){
    qword qwTSC;

    qwTSC = kReadTSC();
    kPrintf("time stamp counter = %q\n", qwTSC);
}

// 프로세서의 속도를 측정
void kMeasureProcessorSpeed(const char* pcParameterBuffer){
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
void kShowDateAndTime(const char* pcParameterBuffer){
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



// TCB 자료구조와 스택 정의
static TCB gs_vstTask[2] = {0, };
static qword gs_vstStack[1024] = {0, };

// 태스크 전환을 테스트하는 태스크
void kTestTask(){
    int i = 0;

    while(true){
        // 메시지 출력하고 키 입력 대기
        kPrintf("[%d] This message is from kTestTask. Press any key to switch "
            "kConsoleShell.\n", i++);
        kGetch();
        
        // 키가 입력되면 태스크 전환
        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}

// 태스크 생성해서 멀티태스킹 수행
void kCreateTestTask(const char* pcParameterBuffer){
    KEYDATA stData;
    int i= 0;

    // 태스크 설정
    kSetUpTask(&(gs_vstTask[1]), 1, 0, (qword)kTestTask, &(gs_vstTask), sizeof(gs_vstStack));

    // 'q'키가 입력될 때까지 수행
    while(true){
        // 메시지 출력하고 키 입력 대기
        kPrintf("[%d] This message is from kConsoleShell. Press any key to "
            "switch TestTask.\n", i++);
        if(kGetch() == 'q'){
            break;
        }
        // 키 입력되면 태스크 전환
        kSwitchContext(&(gs_vstTask[0].stContext), &(gs_vstTask[1].stContext));
    }
}