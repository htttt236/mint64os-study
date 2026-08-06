#include "types.h"
#include "keyboard.h"
#include "descriptor.h"
#include "assembly_utility.h"
#include "utility.h"
#include "pic.h"

void kPrintString(int x, int y, const char* pcString);

void main(){
    kPrintString(0, 10, "switch to IA-32e mode");
    kPrintString(0, 11, "IA-32e C language kernel start");

    kPrintString(0, 12, "GDT initialize and switch for IA-32e mode......");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kPrintString(47, 12, "pass");

    kPrintString(0, 13, "TSS segment load......");
    kLoadTR(GDT_TSSSEGMENT);
    kPrintString(23, 13, "pass");

    kPrintString(0, 14, "IDT nitialize......");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kPrintString(19, 14, "pass");

    kPrintString(0, 15, "keyboard activate and queue initialize......");
    // 키보드 활성화
    if(kInitializeKeyboard() == true){
        kPrintString(44, 15, "pass");
        kChangeKeyboardLED(false, true, false);
    }
    else{
        kPrintString(44, 15, "fail");
        while(true);
    }

    kPrintString(0, 16, "PIC controller and interrupt initialize......");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kPrintString(45, 16, "pass");

    // 키보드 테스트용 shell
    char vcTemp[2] = {0,};
    byte bFlags;
    byte bTemp;
    int i = 0;
    KEYDATA stData;
    while(true){
        // 키 큐에 데이터가 있으면 키를 처리
        if(kGetKeyFromKeyQueue(&stData) == true){

            // 키가 눌렸으면 키의 ASCII 코드 값을 화면에 출력
            if(stData.bFlags & KEY_FLAGS_DOWN){

                // 키 데이터의 ASCII 코드 값을 저장
                vcTemp[0] = stData.bASCIICode;
                kPrintString(i++, 17, vcTemp);

                // 0이 입력되면 Divide Error 예외 발생시킴
                if(vcTemp[0] == '0'){
                    volatile int a = 1;
                    a = bTemp / 0;
                    //__asm__ __volatile__ ("int $0");
                }
            }
        }
    }
}
