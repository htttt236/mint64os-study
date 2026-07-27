#include "types.h"
#include "keyboard.h"

void kPrintString(int x, int y, const char* pcString);

void main(){
    kPrintString(0, 10, "switch to IA-32e mode");
    kPrintString(0, 11, "IA-32e C language kernel start");
    kPrintString(0, 12, "keyboard activate......");

    // 키보드 활성화
    if(kActivateKeyboard() == true){
        kPrintString(23, 12, "pass");
        kChangeKeyboardLED(false, true, false);
    }
    else{
        kPrintString(23, 12, "fail");
        while(true);
    }

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
                    kPrintString(i++, 13, vcTemp);
                }
            }
        }
    }
}

void kPrintString(int x, int y, const char* pcString){
    character_t* pstScreen = (character_t*)0xb8000;
    
    pstScreen += (y*80) + x;

    for(int i=0; pcString[i]!=0; i++){
        pstScreen[i].bCharacter = pcString[i];
    }
}