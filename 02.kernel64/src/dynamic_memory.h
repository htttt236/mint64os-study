#ifndef __DYNAMIC_MEMORy_H__
#define __DYNAMIC_MEMORy_H__

#include "types.h"

// 동적 메모리 영역의 시작 어드레스, 1Mbyte 단위로 정렬
#define DYNAMICMEMORY_START_ADDRESS     ( ( TASK_STACKPOOLADDRESS + \
        ( TASK_STACKSIZE * TASK_MAXCOUNT ) + 0xfffff ) & 0xfffffffffff00000 )
// 버디 블록의 최소 크기, 1KB
#define DYNAMICMEMORY_MIN_SIZE          ( 1 * 1024 )

// 비트맵의 플래그
#define DYNAMICMEMORY_EXIST             0x01
#define DYNAMICMEMORY_EMPTY             0x00


// 비트맵을 관리하는 자료구조
typedef struct kBitmapStruct{
    byte* pbBitmap;
    qword qwExistBitCount;
}BITMAP;

// 버디 블록을 관리하는 자료구조
typedef struct kDynamicMemoryManagerStruct{
    // 블록 리스트의 총 개수와 가장 크기가 작은 블록의 개수, 그리고 할당된 메모리 크기
    int iMaxLevelCount;
    int iBlockCountOfSmallestBlock;
    qword qwUsedSize;

     // 블록 풀의 시작 어드레스와 마지막 어드레스
    qword qwStartAddress;
    qword qwEndAddress;
    
    // 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역과 비트맵 자료구조의 어드레스
    byte* pbAllocatedBlockListIndex;
    BITMAP* pstBitmapOfLevel;
}DYNAMICMEMORY;


void kInitializeDynamicMemory();
void* kAllocateMemory(qword qwSize);
bool kFreeMemory(void* pvAddress);
void kGetDynamicMemoryInformation(qword* pqwDynamicMemoryStartAddress,
        qword* pqwDynamicMemoryTotalSize, qword* pqwMetaDataSize,
        qword* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager();


static qword kCalculateDynamicMemorySize();
static int kCalculateMetaBlockCount(qword qwDynamicRAMSize);
static qword kGetBuddyBlockSize(qword qwSize);
static int kAllocationBuddyBlock(qword qwAlignedSize);
static int kGetBlockListIndexOfMatchSize(qword qwAlignedSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void ksetFlagInBitmap(int iBlockListIndex, int iOffset, byte bFlag);
static bool kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static byte kGetFlagInBitmap(int iBlockListIndex, int iOffset);



#endif /*__DYNAMIC_MEMORy_H__*/