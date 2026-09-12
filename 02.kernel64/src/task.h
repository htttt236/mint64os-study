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

// 준비 리스트의 수
#define TASK_MAXREADYLISTCOUNT  5

// 태스크의 우선 순위
#define TASK_FLAGS_HIGHEST              0
#define TASK_FLAGS_HIGH                 1
#define TASK_FLAGS_MEDIUM               2
#define TASK_FLAGS_LOW                  3
#define TASK_FLAGS_LOWEST               4
#define TASK_FLAGS_WAIT                 0xFF

// 태스크의 플래그
#define TASK_FLAGS_ENDTASK              0x8000000000000000
#define TASK_FLAGS_SYSTEM               0x4000000000000000
#define TASK_FLAGS_PROCESS              0x2000000000000000
#define TASK_FLAGS_THREAD               0x1000000000000000
#define TASK_FLAGS_IDLE                 0x0800000000000000

// 함수 매크로
#define GETPRIORITY( x )        ( ( x ) & 0xFF )
#define SETPRIORITY( x, priority )  ( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | \
        ( priority ) )
#define GETTCBOFFSET( x )       ( ( x ) & 0xFFFFFFFF )

// 자식 스레드 링크에 연결된 stThreadLink 정보에서 태스크 자료구조(TCB) 위치를 
// 계산하여 반환하는 매크로
#define GETTCBFROMTHREADLINK( x )   ( TCB* ) ( ( qword ) ( x ) - offsetof( TCB, \
                                      stThreadLink ) )


#pragma pack(push, 1)

// 콘텍스트 관련 자료구조
typedef struct kContextStruct{
    qword vqRegister[TASK_REGISTERCOUNT];
}CONTEXT;

// 태스크 상태를 관리하는 자료구조
// FPU 콘텍스트가 추가되었기 때문에 자료구조의 크기가 16의 배수로 정렬되어야 함
typedef struct kTaskControlBlockStruct{
    // 다음 데이터의 위치와 ID
    LISTLINK stLink;

    // 플래그
    qword qwFlags;

    // 프로세스 메모리 영역의 시작과 크기
    void* pvMemoryAddress;
    qword qwMemorySize;

    //==========이하 스레드 정보===========

    // 자식 스레드의 위치와 ID
    LISTLINK stThreadLink;

    // 부모 프로세스의 ID
    qword qwParentProcessID;

    // FPU 콘텍스트는 16의 배수로 정렬되어야 하므로, 앞으로 추가할 데이터는 현재 라인
    // 아래에 추가해야 함
    qword vqwFPUContext[512/8];

    // 자식 스레드의 리스트
    LIST stChildThreadList;

    // 콘텍스트와 스택
    CONTEXT stContext;
    void* pvStackAddress;
    qword qwStackSize;

    // FPU 사용 여부
    bool bFPUUsed;

    // TCB 전체를 16바이트 배수로 맞추기 위한 패딩
    byte vcPadding[11];
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

    // 실행할 태스크가 준비 중인 리스트, 태스크의 우선순위에 따라 구분
    LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

    // 종료할 태스크가 대기 중인 리스트
    LIST stWaitList;

    // 각 우선순위별로 태스크를 실행한 횟수를 저장하는 자료구조
    int viExecuteCount[TASK_MAXREADYLISTCOUNT];

    // 프로세서 부하를 계산하기 위한 자료구조
    qword qwProcessorLoad;

    // 유휴 태스크(idle task)에서 사용한 프로세서 시간
    qword qwSpendProcessorTimeInIdleTask;

    // 마지막으로 FPU를 사용한 태스크의 ID
    qword qwLastFPUUsedTaskID;
}SCHEDULER;

#pragma pack(pop)


static void kInitializeTCBPool();
static TCB* kAllocateTCB();
static void kFreeTCB(qword qwID);
TCB* kCreateTask(qword qwFlags, void* pvMemoryAddress, qword qwMemorySize,
                 qword qwEntryPointAddress);
static void kSetUpTask(TCB* pstTCB, qword qwFlags, qword qwEntryPointAddress,
    void* pvStackAddress, qword qwStackSize);

void kInitializeScheduler();
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask();
static TCB* kGetNextTaskToRun();
static bool kAddTaskToReadyList(TCB* pstTask);
static TCB* kRemoveTaskFromReadyList(qword qwTaskID);
bool kChangePriority(qword qwTaskID, byte bPriority);
void kSchedule();
bool kScheduleInInterrupt();
void kDecreaseProcessorTime();
bool kIsProcessorTimeExpired();
bool kEndTask(qword qwTaskID);
void kExitTask();
int kGetReadyTaskCount();
int kGetTaskCount();
TCB* kGetTCBInTCBPool(int iOffset);
bool kIsTaskExist(qword qwID);
qword kGetProcessorLoad();
static TCB* kGetProcessByThread(TCB* pstThread);

void kIdleTask();
void kHaltProcessorByLoad();

qword kGetLastFPUUsedTaskID();
void kSetLastFPUUsedTaskID(qword qwTaskID);


#endif /*__TASK_H__*/