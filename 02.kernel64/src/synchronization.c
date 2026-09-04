#include "synchronization.h"
#include "types.h"
#include "task.h"
#include "utility.h"
#include "assembly_utility.h"


// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
bool kLockForSystemData(){
    return kSetInterruptFlag(false);
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 해제 함수
void kUnlockForSystemData(bool bInterruptFlag){
    kSetInterruptFlag(bInterruptFlag);
}

// 뮤텍스를 초기화
void kInitializeMutex(MUTEX* pstMutex){
    // 잠김 플래그와 횟수, 그리고 태스크 ID를 초기화
    pstMutex->bLockFlag = false;
    pstMutex->dwLockCount = 0;
    pstMutex->qwTaskID = TASK_INVALIDID;
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 함수
void kLock(MUTEX* pstMutex){
    // 이미 잠겨 있다면 내가 잠갔는지 확인하고 잠근 횟수를 증가시킨 뒤 종료
    if(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == false){
        // 자신이 잠갔다면 횟수만 증가시킴
        if(pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID){
            pstMutex->dwLockCount++;
            return;
        }

        // 자신이 아닌 경우는 잠긴 것이 해제될 때까지 대기
        while(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == false){
            kSchedule();
        }
    }

    // 잠김 설정, 잠김 플래그는 위의 kTestAndSet() 함수에서 처리함
    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 해제 함수
void kUnlock(MUTEX* pstMutex){
    // 뮤텍스를 잠근 태스크가 아니면 실패
    if((pstMutex->bLockFlag == false) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID)){
        return;
    }

    // 뮤텍스를 중복으로 잠갔으면 잠긴 횟수만 감소
    if(pstMutex->dwLockCount > 1){
        pstMutex->dwLockCount--;
        return;
    }

    // 해제된 것으로 설정, 잠김 플래그를 가장 나중에 해제해야 함
    pstMutex->qwTaskID = TASK_INVALIDID;
    pstMutex->dwLockCount = 0;
    pstMutex->bLockFlag = false;
}