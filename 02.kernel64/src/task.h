#ifndef __TASK_H__
#define __TASK_H__

#include "types.h"
#include "list.h"


// SS, RSP, RFLAGS, CS, RIP + ISR에서 저장하는 19개의 레지스터
#define TASK_REGISTERCOUNT     (5 + 19)
#define TASK_REGISTERSIZE       8

// Context 자료구조의 레지스터 오프셋
#define TASK_GSOFFSET           0
#define TASK_FSOFFSET           1
#define TASK_ESOFFSET           2
#define TASK_DSOFFSET           3
#define TASK_R15OFFSET          4
#define TASK_R14OFFSET          5
#define TASK_R13OFFSET          6
#define TASK_R12OFFSET          7
#define TASK_R11OFFSET          8
#define TASK_R10OFFSET          9
#define TASK_R9OFFSET           10
#define TASK_R8OFFSET           11
#define TASK_RSIOFFSET          12
#define TASK_RDIOFFSET          13
#define TASK_RDXOFFSET          14
#define TASK_RCXOFFSET          15
#define TASK_RBXOFFSET          16
#define TASK_RAXOFFSET          17
#define TASK_RBPOFFSET          18
#define TASK_RIPOFFSET          19
#define TASK_CSOFFSET           20
#define TASK_RFLAGSOFFSET       21
#define TASK_RSPOFFSET          22
#define TASK_SSOFFSET           23

// 태스크 풀의 어드레스
#define TASK_TCBPOOLADDRESS     0x800000
#define TASK_MAXCOUNT           1024

// 스택 풀과 스택의 크기
#define TASK_STACKPOOLADDRESS   ( TASK_TCBPOOLADDRESS + sizeof( TCB ) * TASK_MAXCOUNT )
#define TASK_STACKSIZE          8192

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID          0xFFFFFFFFFFFFFFFF

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5 ms)
#define TASK_PROCESSORTIME      5


#pragma pack(push, 1)

// 콘텍스트 관련 자료구조
typedef struct kContextStruct{
    qword vqRegister[TASK_REGISTERCOUNT];
}CONTEXT;

// 태스크 상태를 관리하는 자료구조
typedef struct kTaskControlBlockStruct{
    LISTLINK stLink;    // 다음 데이터의 위치와 ID

    qword qwFlags;      // 플래그

    CONTEXT stContext;

    void* pvStackAddress;
    qword qwStackSize;
}TCB;

// TCB 풀의 상태를 관리하는 자료구조
typedef struct kTCBPoolManagerStruct{
    // 태스크 풀에 대한 정보
    TCB* pstStartAddress;
    int iMaxCount;
    int iUseCount;

    // TCB가 할당된 횟수
    int iAllocatedCount;
}TCBPOOLMANAGER;

// 스케줄러의 상태를 관리하는 자료구조
typedef struct kSchedulerStruct{
    // 현재 수행 중인 태스크
    TCB* pstRunningTask;

    // 현재 수행 중인 태스크가 사용할 수 있는 프로세서 시간
    int iProcessorTime;

    // 실행할 태스크가 준비 중인 리스트
    LIST stReadyList;
}SCHEDULER;

#pragma pack(pop)


void kInitializeTCBPool();
TCB* kAllocateTCB();
void kFreeTCB(qword qwID);
TCB* kCreateTask(qword qwFlags, qword qwEntryPointAddress);
void kSetUpTask(TCB* pstTCB, qword qwFlags, qword qwEntryPointAddress,
    void* pvStackAddress, qword qwStackSize);

void kInitializeScheduler();
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun();
void kAddTaskToReadyList(TCB* pstTask);
void kSchedule();
bool kScheduleInInterrupt();
void kDecreaseProcessorTime();
bool kIsProcessorTimeExpired();


#endif /*__TASK_H__*/