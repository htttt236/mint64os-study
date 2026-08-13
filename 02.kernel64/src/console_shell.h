#ifndef __CONSOLE_SHELL__
#define __CONSOLE_SHELL__

#include "types.h"


// 매크로
#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT  300
#define CONSOLESHELL_PROMPTMESSAGE          "os>"

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void (*CommandFunction)(const char* pcParameter);


#pragma pack(push, 1)

// 커맨드 저장용 자료구조
typedef struct kShellCommandEntryStruct{
    char* pcCommand;// 커맨드 문자열
    char* pcHelp;// 커맨드 도움말
    CommandFunction pfFunction;// 커맨드 수행 함수 포인터
}SHELLCOMMANDENTRY;

// 파라미터 정보 저장용 자료구조
typedef struct kParameterListStruct{
    const char* pcBuffer;// 파라미터 버퍼 어드레스
    int iLength;// 파라미터 길이
    int iCurrentPosition;// 현재 처리할 파라미터 시작위치
}PARAMETERLIST;



void kStartConsoleShell();
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);
void kHelp(const char* pcCommandBuffer);
void kCls(const char* pcParameterBuffer);
void kShowTotalRAMSize(const char* pcParameterBuffer);
void kStringToDecimalHexTest(const char* pcParameterBuffer);
void kShutdown(const char* pcParameterBuffer);


#endif /*__CONSOLE_SHELL__*/