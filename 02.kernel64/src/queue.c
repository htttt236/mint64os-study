#include "types.h"
#include "queue.h"
#include "utility.h"


void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, 
    int iDataSize){

    pstQueue->iMaxDataCount = iMaxDataCount;
    pstQueue->iDataSize = iDataSize;
    pstQueue->pvQueueArray = pvQueueBuffer;
    pstQueue->iPutIndex = 0;
    pstQueue->iGetIndex = 0;
}

bool kIsQueueFull(const QUEUE* pstQueue){
    if((pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount == pstQueue->iGetIndex){
        return true;
    }
    return false;
}

bool kIsQueueEmpty(const QUEUE* pstQueue){
    if(pstQueue->iGetIndex == pstQueue->iPutIndex){
        return true;
    }
    return false;
}

bool kPutQueue(QUEUE* pstQueue, const void* pvData){
    if(kIsQueueFull(pstQueue) == true){
        return false;
    }
    //데이터 복사
    kMemCpy((byte*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex), 
        pvData, pstQueue->iDataSize);
    //인덱스 변경
    pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
    return true;
}

bool kGetQueue(QUEUE* pstQueue, void* pvData){
    if(kIsQueueEmpty(pstQueue) == true){
        return false;
    }
    //데이터 복사
    kMemCpy(pvData, (char*)pstQueue->pvQueueArray + 
        (pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize);
    //인덱스 변경
    pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
    return true;
}