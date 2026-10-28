#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include "types.h"
#include "synchronization.h"
#include "harddisk.h"


// 램 디스크의 총 섹터 수. 8MB 크기로 생성
#define RDD_TOTALSECTORCOUNT        (8 * 1024 * 1024 / 512)


// 1 바이트로 정렬
#pragma pack(push, 1)

// 램 디스크의 자료구조를 저장하는 구조체
typedef struct kRDDManagerStruct
{
    // 램 디스크용으로 할당 받은 메모리 영역
    byte* pbBuffer;

    // 총 섹터 수
    dword dwTotalSectorCount;

    // 동기화 객체
    MUTEX stMutex;
}RDDMANAGER;

#pragma pack(pop)


bool kInitializeRDD(dword dwTotalSectorCount);
bool kReadRDDInformation(bool bPrimary, bool bMaster, HDDINFORMATION* pstHDDInformation);
int kReadRDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount,
        char* pcBuffer);
int kWriteRDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount,
        char* pcBuffer);


#endif /*__RAMDISK_H__*/