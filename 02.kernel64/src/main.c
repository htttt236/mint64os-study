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

    kPrintString(0, 15, "keyboard activate......");

    // 키보드 활성화
    if(kActivateKeyboard() == true){
        kPrintString(23, 15, "pass");
        kChangeKeyboardLED(false, true, false);
    }
    else{
        kPrintString(23, 15, "fail");
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
    while(true){
        if(kIsOutputBufferFull() == true){
            bTemp = kGetKeyboardScanCode();
            
            if(kConvertScanCodeToASCIICode(bTemp, &(vcTemp[0]), &bFlags) == true){
                if(bFlags & KEY_FLAGS_DOWN){
                    kPrintString(i++, 17, vcTemp);

                    if(vcTemp[0] == '0'){// 0누르면 Divide Error 발생
                        __asm__ __volatile__ ("int $0");
                    }
                }
            }
        }
    }
}
