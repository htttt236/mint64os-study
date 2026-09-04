#include "types.h"
#include "assembly_utility.h"
#include "keyboard.h"
#include "queue.h"
#include "utility.h"
#include "synchronization.h"

// 출력 버퍼에 수신된 데이터가 있는지 확인
bool kIsOutputBufferFull(){
    if(kInPortByte(0x64) & 0x01){
        return true;
    }
    return false;
}

// 입력 버퍼에 프로세서가 쓴 데이터가 남아있는지 확인
bool kIsInputBufferFull(){
    if(kInPortByte(0x64) & 0x02){
        return true;
    }
    return false;
}

// ack 대기
bool kWaitForACKAndPutOtherScanCode(){
    byte bData;
    bool bResult = false;

    // 키 데이터 고려해서 100개까지 수신
    for(int j=0; j<100; j++){
        //대기
        for(int i=0; i<0xffff; i++){
            if(kIsOutputBufferFull()==true){
                break;
            }
        }
        // ack(0xfa) 확인
        bData = kInPortByte(0x60);
        if(bData == 0xfa){
            bResult = true;
            break;
        }
        else{
            kConvertScanCodeAndPutQueue(bData);
        }
    }
    return bResult;
}

// 키보드 활성화
bool kActivateKeyboard(){
    bool bPreviousInterrupt;
    bool bResult;

    // 인터럽트 막고 이전 인터럽트 상태 저장
    bPreviousInterrupt = kSetInterruptFlag(false);

    kOutPortByte(0x64, 0xae);// 0xae: 키보드 컨트롤러 활성화

    // 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }

    kOutPortByte(0x60,0xf4);// 0xf4: 키보드 활성화

    //ack 대기
    bResult = kWaitForACKAndPutOtherScanCode();
    //이전 인터럽트 상태 복원
    kSetInterruptFlag(bPreviousInterrupt);

    return bResult;
}

// 키 값 읽기
byte kGetKeyboardScanCode(){
    
    // 대기
    while(kIsOutputBufferFull() == false){}

    return kInPortByte(0x60);
}

// A20게이트 활성화
void kEnableA20Gate(){
    byte bOutputPortData;

    kOutPortByte(0x64, 0xd0);// 0xd0: 출력 포트 값을 출력 버퍼(0x60)로 복사

    // 대기
    for(int i=0; i<0xffff; i++){
        if(kIsOutputBufferFull() == true){
            break;
        }
    }
    bOutputPortData = kInPortByte(0x60);

    bOutputPortData |= 0x02;// A20 게이트 활성화 비트(비트 1)

    // 입력 버퍼가 빌 때까지 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }
    kOutPortByte(0x64, 0xd1);// 0xd1: 입력 버퍼 값을 출력 포트로 복사
    kOutPortByte(0x60, bOutputPortData);// 입력 버퍼에 값 전달
}

// 프로세서 리셋
void kReboot(){

    // 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }
    
    kOutPortByte(0x64, 0xd1);// 0xd1: 입력 버퍼 값을 출력 포트로 복사
    kOutPortByte(0x60, 0x00);// 프로세서 초기화 비트 (비트 0)

    while(true);
}

// 상태 led 제어
bool kChangeKeyboardLED(bool bCapsLockOn, bool bNumLockOn, bool bScrollLockOn){
    bool bPreviousInterrupt;
    bool bResult;
    byte bData;

    //인터럽트 막기, 이전 상태 저장
    bPreviousInterrupt = kSetInterruptFlag(false);

    // 커맨드 처리될 때까지 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }

    kOutPortByte(0x60, 0xed);// 0xed: led 상태 변경 커맨드
    //입력 버퍼가 빌 때까지 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }

    // 출력 버퍼에서 ack(0xfa) 읽기
    int j;
    for(j=0; j<100; j++){
        for(int i=0; i<0xffff; i++){
            if(kIsOutputBufferFull() == true){
                break;
            }
        }
        if(kInPortByte(0x60) == 0xfa){
            break;
        }
    }
    if(j>=100){
        return false;
    }

    // led 변경 값 전송
    kOutPortByte(0x60, (bCapsLockOn<<2) | (bNumLockOn<<1) | bScrollLockOn);
    //처리될 때까지 대기
    for(int i=0; i<0xffff; i++){
        if(kIsInputBufferFull() == false){
            break;
        }
    }

    // ack 대기
    bResult = kWaitForACKAndPutOtherScanCode();
    // 이전 상태 복원
    kSetInterruptFlag(bPreviousInterrupt);
    
    return bResult;
}

// 키보드 상태
static KEYBOARDMANAGER gs_stKeyboardManager = { 
    false,  // bShiftDown
    false,  // bCapsLockOn
    true,   // bNumLockOn
    false,  // bScrollLockOn
    false,  // bExtendedCodeIn
    0       // iSkipCountForPause
};
// 키를 저장하는 큐와 버퍼 정의
static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

// 스캔 코드를 ASCII 코드로 변환하는 테이블
static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }
};

// 알파벳 여부 확인
bool kIsAlphabetScanCode(byte bScanCode){
    if(('a'<=gs_vstKeyMappingTable[bScanCode].bNormalCode) && 
    (gs_vstKeyMappingTable[bScanCode].bNormalCode)<='z'){
        return true;
    }
    return false;
}

// 숫자 또는 기호 여부 확인
bool kIsNumberOrSymbolScanCode(byte bScanCode){
    if((2<=bScanCode) && (bScanCode<=53) && 
    (kIsAlphabetScanCode(bScanCode) == false)){
        return true;
    }
    return false;
}

// 숫자 패드 여부 확인
bool kIsNumberPadScanCode(byte bScancode){
    if((71<=bScancode) && (bScancode<=83)){
        return true;
    }
    return false;
}

// 조합 키 사용해야 하는지 여부 확인
bool kIsUseCombinedCode(byte bScanCode){
    byte bDownScanCode;
    bool bUseCombinedKey;

    bDownScanCode = bScanCode & 0x7f;

    // 알파벳일 때
    if(kIsAlphabetScanCode(bDownScanCode) == true){
        if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn){
            bUseCombinedKey = true;
        }
        else{
            bUseCombinedKey = false;
        }
    }

    // 숫자나 기호일 때
    else if(kIsNumberOrSymbolScanCode(bDownScanCode) == true){
        if(gs_stKeyboardManager.bShiftDown == true){
            bUseCombinedKey = true;
        }
        else{
            bUseCombinedKey = false;
        }
    }

    //숫자 패드 키일 때
    else if((kIsNumberPadScanCode(bDownScanCode) == true) && 
    (gs_stKeyboardManager.bExtendedCodeIn == false)){
        if(gs_stKeyboardManager.bNumLockOn == true){
            bUseCombinedKey = true;
        }
        else{
            bUseCombinedKey = false;
        }
    }
    return bUseCombinedKey;
}

// 조합 키 상태 갱신, led 상태 동기화
void UpdateCombinationKeystatusAndLED(byte bScanCode){
    bool bDown;
    byte bDownScanCode;
    bool bLEDStatusChanged = false;

    // 눌림, 떨어짐 상태 처리
    if(bScanCode & 0x80){
        bDown = false;
        bDownScanCode = bScanCode & 0x7f;
    }
    else{
        bDown = true;
        bDownScanCode = bScanCode;
    }

    //키보드 상태 처리
    if((bDownScanCode==42) || (bDownScanCode==54)){// shift
        gs_stKeyboardManager.bShiftDown = bDown;
    }
    else if((bDownScanCode==58) && (bDown==true)){// caps lock
        gs_stKeyboardManager.bCapsLockOn ^= true;
        bLEDStatusChanged = true;
    }
    else if((bDownScanCode==69) && (bDown==true)){// num lock
        gs_stKeyboardManager.bNumLockOn ^= true;
        bLEDStatusChanged = true;
    }
    else if((bDownScanCode==70) && (bDown==true)){// scroll lock
        gs_stKeyboardManager.bScrollLockOn ^= true;
        bLEDStatusChanged = true;
    }

    // led 상태 변경 처리
    if(bLEDStatusChanged == true){
        kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, 
            gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
    }
}

// 스캔 코드를 ASCII 코드로 변환
bool kConvertScanCodeToASCIICode(byte bScanCode, byte* pbASCIICode, byte* pbFlags){
    bool bUseCombinedKey;

    // 이전에 pause가 수신되었으면, 남은 스캔 코드 무시
    if(gs_stKeyboardManager.iSkipCountForPause > 0){
        gs_stKeyboardManager.iSkipCountForPause--;
        return false;
    }

    // pause 키 처리 (0xe1)
    if(bScanCode == 0xE1){
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return true;
    }
    // 확장 키 처리 (0xe0)
    else if(bScanCode == 0xe0){
        gs_stKeyboardManager.bExtendedCodeIn = true;
        return false;
    }


    bUseCombinedKey = kIsUseCombinedCode(bScanCode);
    // 키 값 설정
    if(bUseCombinedKey == true){
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7f].bCombinedCode;
    }
    else{
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7f].bNormalCode;
    }
    // 확장 키 여부 설정
    if(gs_stKeyboardManager.bExtendedCodeIn == true){
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = false;
    }
    else{
        *pbFlags = 0;
    }
    // 눌림, 떨어짐 여부 설정
    if((bScanCode&0x80) == 0){
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    //상태 갱신
    UpdateCombinationKeystatusAndLED(bScanCode);
    return true;
}

// 키보드 초기화
bool kInitializeKeyboard(){

    // 큐 초기화
    kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, 
        sizeof(KEYDATA));
    // 키보드 활성화
    return kActivateKeyboard();
}

// 스캔 코드를 내부적으로 사용하는 키 데이터로 바꾼 후 키 큐에 삽입
bool kConvertScanCodeAndPutQueue(byte bScanCode){
    KEYDATA stData;
    bool bResult = false;
    bool bPreviousInterrupt;

    // 스캔 코드를 키 데이터에 삽입
    stData.bScanCode = bScanCode;

    // 스캔 코드를 ASCII 코드와 키 상태로 변환하여 키 데이터에 삽입
    if(kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), 
        &(stData.bFlags)) == true){
        
        // 임계 영역 시작
        bPreviousInterrupt = kLockForSystemData();
        // 키 큐에 삽입
        bResult = kPutQueue(&gs_stKeyQueue, &stData);
        // 임계 영역 끝
        kUnlockForSystemData(bPreviousInterrupt);
    }
    return bResult;
}

// 키 큐에서 키 데이터를 제거
bool kGetKeyFromKeyQueue(KEYDATA* pstData){
    bool bResult;
    bool bPreviousInterrupt;

    // 큐가 비었으면 키 데이터를 꺼낼 수 없음
    if(kIsQueueEmpty(&gs_stKeyQueue) == true){
        return false;
    }
    // 임계 영역 시작
    bPreviousInterrupt = kLockForSystemData();

    // 키 큐에서 키 데이터 제거
    bResult = kGetQueue(&gs_stKeyQueue, pstData);

    // 임계 영역 끝
    kUnlockForSystemData(bPreviousInterrupt);
    return bResult;
}