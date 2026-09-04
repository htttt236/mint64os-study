#include "task.h"
#include "types.h"
#include "descriptor.h"
#include "utility.h"
#include "assembly_utility.h"
#include "console.h"
#include "synchronization.h"


// 스케줄러 관련 자료구조
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;


//==========================태스크 풀과 태스크 관련=================================

// 태스크 풀 초기화
static void kInitializeTCBPool(){
    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

    // 태스크 풀의 어드레스를 지정하고 초기화
    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    // TCB에 ID 할당
    for(int i=0; i<TASK_MAXCOUNT; i++){
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    // TCB의 최대 개수와 할당된 횟수를 초기화
    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

// TCB를 할당 받음
static TCB* kAllocateTCB(){
    int i;
    TCB* pstEmptyTCB;

    if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount){
        return null;
    }

    for(i=0; i<gs_stTCBPoolManager.iMaxCount; i++){
        //ID의 상위 32비트가 0이면 할당되지 않은 TCB
        if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0){
            pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
            break;
        }
    }

    // 상위 32비트를 0이 아닌 값으로 설정해서 할당된 TCB로 설정
    pstEmptyTCB->stLink.qwID = ((qword)gs_stTCBPoolManager.iAllocatedCount<<32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if(gs_stTCBPoolManager.iAllocatedCount==0){
        gs_stTCBPoolManager.iAllocatedCount = 1;
    }
    
    return pstEmptyTCB;
}

// TCB를 해제함
static void kFreeTCB(qword qwID){
    int i;

    // 태스크 ID의 하위 32비트가 인덱스 역할을 함
    i = GETTCBOFFSET(qwID);

    // TCB를 초기화하고 ID 설정
    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

// 태스크를 생성, 태스크 ID에 따라 스택 풀에서 스택 자동 할당
TCB* kCreateTask(qword qwFlags, qword qwEntryPointAddress){
    TCB* pstTask;
    void* pvStackAddress;
    bool bPreviousFlag;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();
    pstTask = kAllocateTCB();
    if(pstTask == null){
        // 임계 영역 끝
        kUnlockForSystemData(bPreviousFlag);
        return null;
    }
    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);

    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE *
        GETTCBOFFSET(pstTask->stLink.qwID)));
    
    // TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링될 수 있도록 함
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    // 태스크를 준비 리스트에 삽입
    kAddTaskToReadyList(pstTask);

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);

    return pstTask;
}

// 파라미터를 이용해서 TCB 설정
static void kSetUpTask(TCB* pstTCB, qword qwFlags, qword qwEntryPointAddress,
                void* pvStackAddress, qword qwStackSize){
    
    // 콘텍스트 초기화
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // 스택에 관련된 RSP, RBP 레지스터 설정
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (qword)pvStackAddress + qwStackSize;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (qword)pvStackAddress + qwStackSize;

    // 세그먼트 셀렉터 설정
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

    // RIP 레지스터와 인터럽트 플레그 설정
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

    // RFLAGS 레지스터의 IF 비트(비트9)를 1로 설정하여 인터럽트 활성화
    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

    // 스택과 플래그 저장
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}


//=========================스케줄러 관련============================

// 스케줄러 초기화, 필요한 TCB 풀과 init 태스크도 같이 초기화
void kInitializeScheduler(){
    // 태스크 풀 초기화
    kInitializeTCBPool();

    // 준비 리스트와 우선순위별 실행 횟수를 초기화하고 대기 리스트도 초기화
    for(int i=0; i<TASK_MAXREADYLISTCOUNT; i++){
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i] = 0;
    }
    kInitializeList(&(gs_stScheduler.stWaitList));

    // TCB를 할당 받아 실행 중인 태스크로 설정하여, 부팅을 수행한 태스크를 저장할 TCB 준비
    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    // 프로세서 사용률을 계산하는데 사용하는 자료구조 초기화
    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;
}

// 현재 수행 중인 태스크를 설정
void kSetRunningTask(TCB* pstTask){
    bool bPreviousFlag;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    gs_stScheduler.pstRunningTask = pstTask;

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
}

// 현재 수행 중인 태스크를 반환
TCB* kGetRunningTask(void){
    bool bPreviousFlag;
    TCB* pstRunningTask;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    pstRunningTask = gs_stScheduler.pstRunningTask;

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);

    return pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크를 얻음
static TCB* kGetNextTaskToRun(){
    TCB* pstTarget = null;
    int iTaskCount, i, j;

    // 큐에 태스크가 있으나 모든 큐의 태스크가 1회씩 실행된 경우 모든 큐가 프로세서를 양보하여
    // 태스크를 선택하지 못할 수 있으니 null일 경우 한 번 더 수행
    for(j=0; j<2; j++){
        // 높은 우선순위에서 낮은 우선순위까지 리스트를 확인하여 스케줄링할 태스크를 선택
        for(i=0; i<TASK_MAXREADYLISTCOUNT; i++){
            iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

            // 만약 실행한 횟수보다 리스트의 태스크 수가 더 많으면 현재 우선순위의 태스크를 실행
            if(gs_stScheduler.viExecuteCount[i] < iTaskCount){
                pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
                gs_stScheduler.viExecuteCount[i]++;
                break;
            }
            // 만약 실행한 횟수가 더 많으면 실행 횟수를 초기화하고 다음 우선순위로 양보
            else{
                gs_stScheduler.viExecuteCount[i] = 0;
            }
        }

        // 만약 수행할 태스크를 찾았으면 종료
        if(pstTarget != null){
            break;
        }
    }
    return pstTarget;
}

// 태스크를 스케줄러의 준비 리스트에 삽입
static bool kAddTaskToReadyList(TCB* pstTask){
    byte bPriority;

    bPriority = GETPRIORITY(pstTask->qwFlags);
    if(bPriority >= TASK_MAXREADYLISTCOUNT){
        return false;
    }

    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
    return true;
}

// 준비 큐에서 태스크 제거
static TCB* kRemoveTaskFromReadyList(qword qwTaskID){
    TCB* pstTarget;
    byte bPriority;
    // 태스크 ID가 유효하지 않으면 실패
    if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT){
        return null;
    }

    // TCB 풀에서 해당 태스크의 TCB를 찾아 ID 일치하는지 확인
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if(pstTarget->stLink.qwID != qwTaskID){
        return null;
    }

    // 태스크가 존재하는 준비 리스크에서 태스크 제거
    bPriority = GETPRIORITY(pstTarget->qwFlags);
    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);

    return pstTarget;
}

// 태스크 우선순위 변경
bool kChangePriority(qword qwTaskID, byte bPriority){
    TCB* pstTarget;
    bool bPreviousFlag;

    if(bPriority >= TASK_MAXREADYLISTCOUNT){
        return false;
    }

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    // 현재 실행 중인 태스크면 우선순위만 변경
    // PIT 컨트롤러의 인터럽트(IRQ 0)가 발생하여 태스크 전환이 수행될 때 변경된
    // 우선순위의 리스트로 이동
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID){
        SETPRIORITY(pstTarget->qwFlags, bPriority);
    }
    // 실행 중인 태스크가 아니면 준비 리스트에서 찾아서 해당 우선순위의 리스트로 이동
    else{
        // 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 우선순위로 설정
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == null){
            // 태스크 ID로 직접 찾아서 설정
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != null){
                // 우선순위를 설정
                SETPRIORITY(pstTarget->qwFlags, bPriority);
            }
        }
        else{
            // 우선순위를 설정하고 준비 리스트에 다시 삽입
            SETPRIORITY(pstTarget->qwFlags, bPriority);
            kAddTaskToReadyList(pstTarget);
        }
    }
    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
    return true;
}

// 다른 태스크를 찾아서 전환, 인터럽트나 예외 발생 시 호출 금지
void kSchedule(){
    TCB* pstRunningTask, *pstNextTask;
    bool bPreviousFlag;

    // 전환할 태스크가 있어야 함
    if(kGetReadyTaskCount() < 1){
        return;
    }

    // 전환 도중 태스크 전환이 일어나지 않도록 인터럽트 발생하지 못하도록 설정
    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();
    // 실행할 다음 태스크 얻기
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == null){
        // 임계 영역 끝
        kUnlockForSystemData(bPreviousFlag);
        return;
    }

    
    // 현재 수행 중인 태스크의 정보를 수정한 콘텍스트 전환
    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    // 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
        gs_stScheduler.qwSpendProcessorTimeInIdleTask +=
            TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    // 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장할 필요가 없으므로, 대기 리스트에
    // 삽입하고 콘텍스트 전환
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK){
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kSwitchContext(null, &(pstNextTask->stContext));
    }
    else{
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    // 프로세서 사용 시간을 업데이트
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    
    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
}

// 인터럽트 발생 시 다른 태스크 찾아 전환, 반드시 인터럽트나 예외 발생 시 호출
bool kScheduleInInterrupt(){
    TCB* pstRunningTask, *pstNextTask;
    char* pcContextAddress;
    bool bPreviousFlag;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    // 전환할 태스크가 없으면 종료
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == null){
        // 임계 영역 끝
        kUnlockForSystemData(bPreviousFlag);
        return false;
    }

    //               ============태스크 전환 처리=============
    // 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방식으로 처리

    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    // 현재 수행 중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    // 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
    }

    // 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장하지 않고 대기 리스트에만 삽입
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK){
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    }
    // 태스크가 종료되지 않으면 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김
    else{
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }
    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사해서
    // 자동으로 태스크 전환이 일어나도록 함
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    // 프로세서 사용 시간을 업데이트
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    return true;
}

// 프로세서를 사용할 수 있는 시간을 하나 줄임
void kDecreaseProcessorTime(){
    if(gs_stScheduler.iProcessorTime > 0){
        gs_stScheduler.iProcessorTime--;
    }
}

// 프로세서를 사용할 수 있는 시간이 다 되었는지 여부를 반환
bool kIsProcessorTimeExpired(){
    if(gs_stScheduler.iProcessorTime <= 0){
        return true;
    }
    return false;
}

// 태스크를 종료
bool kEndTask(qword qwTaskID){
    TCB* pstTarget;
    byte bPriority;
    bool bPreviousFlag;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    // 현재 실행 중인 태스크면 EndTask 비트를 설정하고 태스크를 전환
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID){
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        // 임계 영역 끝
        kUnlockForSystemData(bPreviousFlag);

        kSchedule();

        // 태스크가 전환되었으므로 아래 코드는 절대 실행되지 않음
        while(true);
    }
    // 실행 중인 태스크가 아니면 준비 큐에서 직접 찾아서 대기 리스트에 연결
    else{
        // 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 태스크 종료 비트를 설정
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == null){
            // 태스크 ID로 직접 찾아서 설정
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if((pstTarget != null) && (pstTarget->stLink.qwID == qwTaskID)){
                pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
                SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

                // 임계 영역 끝
                kUnlockForSystemData(bPreviousFlag);
                return true;
            }
            // 임계 영역 끝
            kUnlockForSystemData(bPreviousFlag);
            return false;
        }

        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }
    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
    return true;
}

// 태스크가 자신을 종료
void kExitTask(){
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

// 준비 큐에 있는 모든 태스크 수를 반환
int kGetReadyTaskCount(){
    int iTotalCount = 0;
    bool bPreviousFlag;

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    // 모든 준비 큐를 확인하여 태스크 개수를 구함
    for(int i=0; i<TASK_MAXREADYLISTCOUNT; i++){
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}

// 전체 태스크의 수를 반환
int kGetTaskCount(){
    int iTotalcount;
    bool bPreviousFlag;

    // 준비 큐의 테스크 수를 구한 후, 대기 큐의 태스크 수와 현재 수행 중인 태스크 수를 더함
    iTotalcount = kGetReadyTaskCount();

    // 임계 영역 시작
    bPreviousFlag = kLockForSystemData();

    iTotalcount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousFlag);
    return iTotalcount;
}

// TCB 풀에서 해당 오프셋의 TCB를 반환
TCB* kGetTCBInTCBPool(int iOffset){
    if((iOffset < 0) || (iOffset >= TASK_MAXCOUNT)){
        return null;
    }

    return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

// 태스크가 존재하는지 여부를 반환
bool kIsTaskExist(qword qwID){
    TCB* pstTCB;

    // ID로 TCB를 반환
    pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
    // TCB가 없거나 ID가 일치하지 않으면 존재하지 않는 것임
    if((pstTCB == null) || (pstTCB->stLink.qwID != qwID)){
        return false;
    }
    return true;
}

// 프로세서의 사용률을 반환
qword kGetProcessorLoad(){
    return gs_stScheduler.qwProcessorLoad;
}

//=======================유휴 태스크 관련==========================

// 유휴 태스크
// 대기 큐에 삭제 대기중인 태스크를 정리
void kIdleTask(){
    TCB* pstTask;
    qword qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    qword qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    bool bPreviousFlag;
    qword qwTaskID;

    // 프로세서 사용량 계산을 위해 기준 정보를 저장
    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(true){
        // 현재 상태를 저장
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        // 프로세서 사용량을 계산
        // 100 - (idle task가 사용한 프로세서 시간) * 100 / (시스템 전체에서 사용한 프로세서 시간)
        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0){
            gs_stScheduler.qwProcessorLoad = 0;
        }
        else{
            gs_stScheduler.qwProcessorLoad = 100 -
                (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
                100 /(qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        // 현재 상태를 이전 상태에 보관
        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        // 프로세서 부하에 따라 쉬게 함
        kHaltProcessorByLoad();

        // 대기 큐에 대기중인 태스크가 있으면 태스크를 종료함
        if(kGetListCount(&(gs_stScheduler.stWaitList)) > 0){
            while(true){
                // 임계 영역 시작
                bPreviousFlag = kLockForSystemData();
                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if(pstTask == null){
                    // 임계 영역 끝
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }
                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);
                // 임계 영역 끝
                kUnlockForSystemData(bPreviousFlag);
                kPrintf("idle: task ID[0x%q] is completely ended.\n", pstTask->stLink.qwID);
                
            }
        }
        kSchedule();
    }
}

// 측정된 프로세서 부하에 따라 프로세서를 쉬게 함
void kHaltProcessorByLoad(){
    if(gs_stScheduler.qwProcessorLoad < 40){
        kHlt();
        kHlt();
        kHlt();
    }
    else if(gs_stScheduler.qwProcessorLoad < 80){
        kHlt();
        kHlt();
    }
    else if(gs_stScheduler.qwProcessorLoad < 95){
        kHlt();
    }
}