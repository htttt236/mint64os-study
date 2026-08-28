#include "list.h"


// 리스트 초기화
void kInitializeList(LIST* pstList){
    pstList->iItemCount = 0;
    pstList->pvHeader = null;
    pstList->pvTail = null;
}

// 리스트에 포함된 아이템의 수를 반환
int kGetListCount(const LIST* pstList){
    return pstList->iItemCount;
}

// 리스트에 데이터를 더함
void kAddListToTail(LIST* pstList, void* pvItem){
    LISTLINK* pstLink;

    // 다음 데이터의 어드레스를 null로 설정
    pstLink = (LISTLINK*) pvItem;
    pstLink->pvNext = null;

    // 리스트가 빈 상태면 Header와 Tail을 추가한 데이터로 설정
    if(pstList->pvHeader == null){
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;

        return;
    }

    // 마지막 데이터의 LISTLINK의 위치를 구하여 다음 데이터를 추가한 데이터로 설정
    pstLink = (LISTLINK*)pstList->pvTail;
    pstLink->pvNext = pvItem;

    // 리스트의 마지막 데이터를 추가한 데이터로 변경
    pstList->pvTail = pvItem;
    pstList->iItemCount++;
}

// 리스트의 첫 부분에 데이터를 더함
void kAddListToHeader(LIST* pstList, void* pvItem){
    LISTLINK* pstLink;

    // 다음 데이터의 어드레스를 Header로 설정
    pstLink = (LISTLINK*)pvItem;
    pstLink->pvNext = pstList->pvHeader;

    // 리스트가 빈 상태이면 Header와 Tail을 추가한 데이터로 설정
    if (pstList->pvHeader == null){
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;

        return;
    }

    // 리스트의 첫 번째 데이터를 추가한 데이터로 변경
    pstList->pvHeader = pvItem;
    pstList->iItemCount++;
}

// 리스트에서 데이터를 제거한 후 데이터의 포인터를 반환
void* kRemoveList(LIST* pstList, qword qwID){
    LISTLINK* pstLink;
    LISTLINK* pstPreviousLink;

    pstPreviousLink = null;
    for(pstLink = (LISTLINK*)pstList->pvHeader; pstLink != null; pstLink = pstLink->pvNext){
        // 일치하는 ID가 있다면 제거
        if(pstLink->qwID == qwID){
            if(pstPreviousLink == null){
                pstList->pvHeader = pstLink->pvNext;
            }
            else{
                pstPreviousLink->pvNext = pstLink->pvNext;
            }
            
            if(pstLink->pvNext == null){
                pstList->pvTail = pstPreviousLink;
            }

            pstList->iItemCount--;
            return pstLink;
        }
        pstPreviousLink = pstLink;
    }
    return null;
}

// 리스트의 첫 번째 데이터를 제거하여 반환
void* kRemoveListFromHeader(LIST* pstList){
    LISTLINK* pstLink;

    if(pstList->iItemCount == 0){
        return null;
    }

    // 헤더를 제거하고 반환
    pstLink = (LISTLINK*)pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}

// 리스트의 마지막 데이터를 제거하여 반환
void* kRemoveListFromTail(LIST* pstList){
    LISTLINK* pstLink;

    if(pstList->iItemCount == 0){
        return null;
    }

    // 테일을 제거하고 반환
    pstLink = (LISTLINK*) pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}

//리스트에서 아이템을 찾음
void* kFindList(const LIST* pstList, qword qwID){
    LISTLINK* pstLink;

    for(pstLink = (LISTLINK*)pstList->pvHeader; pstLink != null; pstLink = pstLink->pvNext){
        // 일치하는 게 있다면 반환
        if(pstLink->qwID == qwID){
            return pstLink;
        }
    }
    return null;
}

// 리스트의 헤더를 반환
void* kGetHeaderFromList(const LIST* pstList){
    return pstList->pvHeader;
}

// 리스트의 테일을 반환
void* kGetTailFromList(const LIST* pstList){
    return pstList->pvTail;
}

// 현재 아이템의 다음 아이템을 반환
void* kGetNextFromList(const LIST* pstList, void* pstCurrent){
    LISTLINK* pstLink;

    pstLink = (LISTLINK*)pstCurrent;

    return pstLink->pvNext;
}