#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "types.h"

#pragma pack(push, 1)

/* mint64os 큐
typedef struct kQueueManagerStruct{
    int iDataSize;
    int iMaxDataCount;

    void* pvQueueArray;
    int iPutIndex;
    int iGetIndex;

    bool bLastOperationPut;
}QUEUE;
*/

typedef struct kQueueManagerStruct{
    // bool bLastOperationPut 삭제 후 버퍼 한 칸 비우고 사용
    int iDataSize;
    int iMaxDataCount;

    void* pvQueueArray;
    int iPutIndex;
    int iGetIndex;

}QUEUE;

#pragma pack(pop)

void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
bool kIsQueueFull(const QUEUE* pstQueue);
bool kIsQueueEmpty(const QUEUE* pstQueue);
bool kPutQueue(QUEUE* pstQueue, const void* pvData);
bool kGetQueue(QUEUE* pstQueue, void* pvData);

#endif /*__QUEUE_H__*/