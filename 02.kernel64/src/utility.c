#include "utility.h"
#include "types.h"

void kPrintString(int x, int y, const char* pcString){
    character_t* pstScreen = (character_t*)0xb8000;
    
    pstScreen += (y*80) + x;

    for(int i=0; pcString[i]!=0; i++){
        pstScreen[i].bCharacter = pcString[i];
    }
}


// 특정 값으로 메모리 채우기
void kMemSet(void* pvDestination, byte bData, int iSize){
    for(int i=0; i<iSize; i++){
        ((byte*)pvDestination)[i] = bData;
    }
}

// 메모리 복사
int kMemCpy(void* pvDestination, const void* pvSource, int iSize){
    for(int i=0; i<iSize; i++){
        ((byte*)pvDestination)[i] = ((byte*)pvSource)[i];
    }
    return iSize;
}

// 메모리 비교
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize){
    int cTemp;
    for(int i=0; i<iSize; i++){
        cTemp = (int)((byte*)pvDestination)[i] - (int)((byte*)pvSource)[i];
        if(cTemp != 0){
            return (int)cTemp;
        }
    }
    return 0;
}