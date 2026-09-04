#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "types.h"


#pragma pack(push, 1)

// 뮤텍스 자료구조
typedef struct kMutexStruct{
    // 태스크 ID와 잠금을 수행한 횟수
    volatile qword qwTaskID;
    volatile dword dwLockCount;

    // 잠금 플래그
    volatile bool bLockFlag;

    // 자료구조의 크기를 8바이트 단위로 맞추려고 추가한 필드
    byte vbPadding[3];
}MUTEX;

#pragma pack(pop)


bool kLockForSystemData();
void kUnlockForSystemData(bool bInterruptFlag);
void kInitializeMutex(MUTEX* pstMutex);
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);



#endif /*__SYNCHRONIZATION_H__*/