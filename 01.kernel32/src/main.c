#include "types.h"

void kPrintString(int x, int y, const char* pcString);
bool kInitializeKernel64Area();
bool kIsMemoryEnough();

void main(){
    kPrintString(0, 3, "C language kernel start");

    kPrintString(0, 4, "memory size check......");
    if(kIsMemoryEnough()){
        kPrintString(23, 4, "pass");
    }else{
        kPrintString(23, 4, "not enough memory");
        goto END;
    }

    kPrintString(0, 5, "IA-32e kernel area initialize......");
    if(kInitializeKernel64Area()){
        kPrintString(35, 5, "pass");
    }else{
        kPrintString(23, 5, "fail");
        goto END;
    }

END:
    while(true);
}

void kPrintString(int x, int y, const char* pcString){
    character_t* pstScreen = (character_t*)0xb8000;
    int i;
    
    pstScreen += (y*80) + x;
    for(i=0; pcString[i]!=0; i++){
        pstScreen[i].character = pcString[i];
    }
}

bool kInitializeKernel64Area(){
    volatile dword* pdwCurrentAdress = (volatile dword*)0x100000;

    while((dword)pdwCurrentAdress < 0x600000){
        *pdwCurrentAdress = 0x00;

        if(*pdwCurrentAdress != 0) return false;
        pdwCurrentAdress++;
    }
    return true;
}

bool kIsMemoryEnough(){
    volatile dword* pdwCurrentAdress = (volatile dword*)0x100000;

    while((dword)pdwCurrentAdress < 0x4000000){
        *pdwCurrentAdress = 0x12345678;

        if(*pdwCurrentAdress != 0x12345678) return false;
        pdwCurrentAdress += (0x100000/4);
    }
    return true;
}