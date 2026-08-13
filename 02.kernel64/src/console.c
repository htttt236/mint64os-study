#include <stdarg.h>
#include "console.h"
#include "keyboard.h"
#include "utility.h"
#include "assembly_utility.h"

// 콘솔 정보를 관리하는 자료구조
CONSOLEMANAGER gs_stConsoleManager = {0, };

// 콘솔 초기화
void kInitializeConsole(int iX, int iY){
    kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
    kSetCursor(iX, iY);
}

// 커서 위치, 문자 출력 위치 설정
void kSetCursor(int iX, int iY){
    int iLinearValue;

    //커서 위치 계산
    iLinearValue = iY * CONSOLE_WIDTH + iX;

    // CRCT 컨트롤 레지스터(포트 0x3d5)에 커서의 상위 바이트를 출력
    // 상위 커서 위치 레지스터 선택
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
    // CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 상위 바이트를 출력
    kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

    // CRTC 컨트롤 어드레스 레지스터(포트 0x3D4)에 0x0F를 전송하여
    // 하위 커서 위치 레지스터를 선택
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
    // CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 하위 바이트를 출력
    kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

    //출력 위치 업데이트
    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

//커서 위치 반환
void kGetCursor(int* piX, int* piY){
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

// printf 내부구현
void kPrintf(const char* pcFormatString, ...){
    va_list ap;
    char vcBuffer[100];
    int iNextPrintOffset;

    // vsprintf로 처리
    va_start(ap, pcFormatString);// pcFormatString 이후부터 가변 인자로 지정
    kVSPrintf(vcBuffer, pcFormatString, ap);
    va_end(ap);

    // 화면에 출력
    iNextPrintOffset = kConsolePrintString(vcBuffer);

    //커서 위치 업데이트
    kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

// 화면에 문자열 출력 후 다음 위치 반환
int kConsolePrintString(const char* pcBuffer){
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    int iLength = kStrLen(pcBuffer);
    int iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

    for(int i=0; i<iLength; i++){

        // 출력 위치가 최댓값(80*25)을 벗어나면 스크롤 처리
        if(iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH)){
            //맨 윗줄 제외하고 한 줄 위로 복사
            kMemCpy(CONSOLE_VIDEOMEMORYADDRESS, 
                CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER), 
                (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));
            //마지막줄 공백으로 채우기
            for(int j = (CONSOLE_HEIGHT-1) * CONSOLE_WIDTH;
                j < CONSOLE_HEIGHT * CONSOLE_WIDTH;
                j++){
                pstScreen[j].bCharacter = ' ';
                pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            }
            //출력 위치 조정
            iPrintOffset -= CONSOLE_WIDTH;
        }

        if(pcBuffer[i] == '\n'){// 개행 처리
            iPrintOffset += CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH);
        }
        else if(pcBuffer[i] == '\t'){// 탭 처리
            // 다음 8의 배수로 옮김
            iPrintOffset += 8 - (iPrintOffset % 8);
        }
        else if(pcBuffer[i] == KEY_BACKSPACE){// backspace 처리
            //커서 한 칸 전으로
            iPrintOffset--;
        }
        else{// 일반 문자열 출력
            pstScreen[iPrintOffset].bCharacter = pcBuffer[i];
            pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset++;
        }
    }
    return iPrintOffset;
}

// 전체 화면 삭제
void kClearScreen(){
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

    for(int i=0; i<CONSOLE_WIDTH * CONSOLE_HEIGHT; i++){
        pstScreen[i].bCharacter = ' ';
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }

    kSetCursor(0, 0);
}

// getch() 함수 구현
byte kGetch(){
    KEYDATA stData;

    //키가 눌릴 때까지 대기
    while(true){
        // 키 큐에 데이터가 수신될 때까지 대기
        while(kGetKeyFromKeyQueue(&stData) == false){}

        // 키가 눌렸다는 데이터가 수신되면 ASCII 코드를 반환
        if(stData.bFlags & KEY_FLAGS_DOWN){
            return stData.bASCIICode;
        }
    }
}

// 문자열을 X, Y 위치에 출력 (utility.c의 kPrintString()을 옮긴 함수)
void kPrintStringXY(int iX, int iY, const char* pcString){
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    
    // 출력 위치 계산
    pstScreen += (iY*80) + iX;

    // 메모리에 문자 쓰기
    for(int i=0; pcString[i] != 0; i++){
        pstScreen[i].bCharacter = pcString[i];
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
}