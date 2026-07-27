#include "types.h"
#include "page.h"
#include "mode_switch.h"

void kPrintString(int x, int y, const char* pcString);
bool kInitializeKernel64Area();
bool kIsMemoryEnough();
void kCopyKernel64ImageTo2Mbyte();

void main(){
    dword dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[13] = {0};

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

    kPrintString(0, 6, "IA-32e page tables initalize......");
    kInitializePageTables();
    kPrintString(34, 6, "pass");

    //프로세서 제조사 정보 읽기
    kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(dword*)vcVendorString = dwEBX;
    *((dword*)vcVendorString+1) = dwEDX;
    *((dword*)vcVendorString+2) = dwECX;
    kPrintString(0, 7, "processor vendor string......");
    kPrintString(29, 7, vcVendorString);

    //64비트 지원 유무 확인
    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    kPrintString(0, 8, "64bit mode support check......");
    if(dwEDX & (1<<29)){
        kPrintString(30, 8, "pass");
    }else{
        kPrintString(30, 8, "fail");
        kPrintString(0, 9, "this processor does not support 64bit");
        goto END;
    }

    //IA-32e 커널을 0x200000(2MB)로 이동
    kPrintString(0, 9, "copy IA-32e kernel to 2M address......");
    kCopyKernel64ImageTo2Mbyte();
    kPrintString(38, 9, "pass");

    
    //IA-32e 모드 전환
    kPrintString(0,10, "switch to IA-32e mode");
    kSwitchAndExecute64bitKernel();

END:
    while(true);
}

void kPrintString(int x, int y, const char* pcString){
    character_t* pstScreen = (character_t*)0xb8000;
    int i;
    
    pstScreen += (y*80) + x;
    for(i=0; pcString[i]!=0; i++){
        pstScreen[i].bCharacter = pcString[i];
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

void kCopyKernel64ImageTo2Mbyte(){
    word wKernel32SectorCount, wTotalKernelSectorCount;
    dword* pdwSourceAddress, *pdwDestinationAddress;

    //0x7c05에 총 커널 섹터 수, 0x7c07에 보호 모드 커널 섹터 수 있음
    wTotalKernelSectorCount = *((word*)0x7c05);
    wKernel32SectorCount = *((word*)0x7c07);

    pdwSourceAddress = (dword*)(0x10000 + (wKernel32SectorCount*512));
    pdwDestinationAddress = (dword*)0x200000;

    //커널 복사
    for(int i=0; i < 512*(wTotalKernelSectorCount-wKernel32SectorCount)/4; i++){
        *pdwDestinationAddress = *pdwSourceAddress;
        pdwDestinationAddress++;
        pdwSourceAddress++;
    }
}