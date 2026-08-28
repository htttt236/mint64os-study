#ifndef __LIST_H__
#define __LIST_H__

#include "types.h"


#pragma pack(push, 1)

// 데이터를 연결하는 자료구조
// 반드시 데이터의 가장 앞부분에 위치해야 함
typedef struct kListLinkStruct{
    void* pvNext;   // 다음 데이터 주소
    qword qwID;
}LISTLINK;

// 리스트를 관리하는 자료구조
typedef struct kListManagerStruct{
    int iItemCount; // 리스트 데이터의 수
    void* pvHeader; // 첫 번째 데이터 주소
    void* pvTail;   // 마지막 데이터 주소
}LIST;


void kInitializeList(LIST* pstList);
int kGetListCount(const LIST* pstList);
void kAddListToTail(LIST* pstList, void* pvItem);
void kAddListToHeader(LIST* pstList, void* pvItem);
void* kRemoveList(LIST* pstList, qword qwID);
void* kRemoveListFromHeader(LIST* pstList);
void* kRemoveListFromTail(LIST* pstList);
void* kFindList(const LIST* pstList, qword qwID);
void* kGetHeaderFromList(const LIST* pstList);
void* kGetTailFromList(const LIST* pstList);
void* kGetNextFromList(const LIST* pstList, void* pstCurrent);



#endif /*__LIST_H__*/