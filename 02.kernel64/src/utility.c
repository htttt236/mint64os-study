#include "utility.h"
#include "types.h"
#include "assembly_utility.h"
#include <stdarg.h>

// PIT 컨트롤러가 발생한 횟수를 저장할 카운터
volatile qword g_qwTickCount = 0;

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

// rflags 레지스터의 인터럽트 플래그를 변경하고 이전 인터럽트 플래그의 상태를 반환
bool kSetInterruptFlag(bool bEnableInterrupt){
    qword qwRFLAGS;

    qwRFLAGS = kReadRFLAGS();// 이전 값 저장
    if(bEnableInterrupt == true){
        kEnableInterrupt();
    }
    else{
        kDisableInterrupt();
    }

    if(qwRFLAGS & 0x0200){// 이전의 인터럽트 상태를 반환
        return true;
    }
    return false;
}

// 문자열의 길이 반환
int kStrLen(const char* pcBuffer){
    int i;
    for(i=0; pcBuffer[i] != '\0'; i++){}

    return i;
}

// 램의 총 크기(MB 단위)
static qword gs_qwTotalRAMMBSize = 0;

// 64MB 이상의 위치부터 램 크기 체크, 최초 부팅 과정에서 한번만 호출해야 함
void kCheckTotalRAMSize(){
    volatile dword* pdwCurrentAddress;
    dword dwPreviouValue;

    // 64MB(0x4000000)부터 4MB단위로 검사 시작
    pdwCurrentAddress = (volatile dword*)0x4000000;
    while(1){
        // 메모리에 있는 기존 값을 저장
        dwPreviouValue = *pdwCurrentAddress;

        //0x12345678을 써서 읽었을 때 문제가 없는 곳까지 유효한 영역으로 인정
        *pdwCurrentAddress = 0x12345678;
        if(*pdwCurrentAddress != 0x12345678){
            break;
        }
        // 이전 메모리 값으로 복원
        *pdwCurrentAddress = dwPreviouValue;
        // 다음 4MB 위치로 이동
        pdwCurrentAddress += (0x400000 / 4);
    }
    // 체크가 성공한 어드레스를 1MB로 나누어 MB 단위로 계산
    gs_qwTotalRAMMBSize = (qword) pdwCurrentAddress / 0x100000;
}

// RAM 크기 반환
qword kGetTotalRAMSize(){
    return gs_qwTotalRAMMBSize;
}

// atoi() 내부구현
long kAToI(const char* pcBuffer, int iRadix){
    long lReturn;

    switch(iRadix){
    case 16:// 16진수
        lReturn = kHexStringToQword(pcBuffer);
        break;
    
    case 10:// 10진수
    default:
        lReturn = kDecimalStringToLong(pcBuffer);
        break;
    }
    return lReturn;
}

// 16진수 문자열을 qword로 변환
qword kHexStringToQword(const char* pcBuffer){
    qword qwValue = 0;
    char cTemp;
    
    for(int i=0; pcBuffer[i] != '\0'; i++){
        qwValue <<= 4;
        cTemp = pcBuffer[i];

        if((cTemp >= 'A' && cTemp <= 'F') || (cTemp >= 'a' && cTemp <= 'f')){
            // 'A' ~ 'F' (0x41 ~ 0x46) or 'a' ~ 'f' (0x61 ~ 0x66)
            qwValue += (cTemp & 0x0f) + 9;
        }
        else{
            // '0' ~ '9' (0x30 ~ 0x39)
            qwValue += (cTemp & 0x0f);
        }
    }
    return qwValue;
}

long kDecimalStringToLong(const char* pcBuffer){
    unsigned long lValue = 0;// 오버플로우 방지
    int i = 0;
    bool bIsNagative = 0;

    if(pcBuffer[0] == '-'){
        bIsNagative = true;
        i = 1;
    }

    for(; pcBuffer[i] != '\0'; i++){
        lValue *= 10;
        lValue += pcBuffer[i] & 0x0f;
    }

    if(bIsNagative){
        lValue = -(long)lValue;
    }
    return lValue;
}

// itoa() 내부구현
int kIToA(long lValue, char* pcBuffer, int iRadix){
    int iReturn;

    switch(iRadix){
    case 16:// 16진수
        iReturn = kHexToString(lValue, pcBuffer);
        break;
    
    case 10:// 10진수
    default:
        iReturn = kDecimalToString(lValue, pcBuffer);
        break;
    }
    return iReturn;
}

// 16진수 값을 문자열로 변환
int kHexToString(qword qwValue, char* pcBuffer){
    static const char vcHexTable[] = "0123456789ABCDEF";
    int i;
    qword qwCurrentValue;

    if(qwValue == 0){// 0은 바로 처리
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    // 한 자리씩 버퍼에 삽입
    for(i=0; qwValue>0; i++){
        pcBuffer[i] = vcHexTable[qwValue & 0x0f];
        qwValue >>= 4;
    }
    pcBuffer[i] = '\0';

    // 버퍼에 있는 문자열 뒤집기
    kReverseStringLen(pcBuffer, i);
    return i;
}

// 10진수 값을 문자열로 변환
int kDecimalToString(long lValue, char* pcBuffer){
    unsigned long ulValue;// lValue가 최솟값일 때 생기는 문제 방지용
    int i = 0;

    // 0은 바로 처리
    if(lValue == 0){
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    // 음수면 출력 버퍼에 '-'추가하고 양수로 변환
    if(lValue < 0){
        i = 1;
        pcBuffer[0] = '-';
        ulValue = -(unsigned long)lValue;
    }
    else{
        ulValue = (unsigned long)lValue;
    }

    for(; ulValue>0; i++){
        pcBuffer[i] = '0' + ulValue % 10;
        ulValue = ulValue / 10;
    }
    pcBuffer[i] = '\0';
    if(pcBuffer[0] == '-'){
        kReverseStringLen(&(pcBuffer[1]), i-1);
    }
    else{
        kReverseStringLen(pcBuffer, i);
    }
    return i;
}

// 문자열의 순서를 뒤집음
void kReverseStringLen(char* pcBuffer, int iLength){
    char cTemp;
    for(int i=0; i<iLength/2; i++){
        cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength-1-i];
        pcBuffer[iLength-1-i] = cTemp;
    }
}
void kReverseString(char* pcBuffer){
    kReverseStringLen(pcBuffer, kStrLen(pcBuffer));
}

// sprintf() 함수 내부구현
int kSprintf(char* pcBuffer, const char* pcFormatString, ...){
    va_list ap;
    int iReturn;

    // 가변 인자를 꺼내서 vsprintf()에 넘겨줌
    va_start(ap, pcFormatString);
    iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
    va_end(ap);

    return iReturn;
}

// vsprintf() 함수의 내부구현
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap){
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char* pcCopyString;
    qword qwValue;
    int iValue;

    // format string 길이 읽어서 길이만큼 데이터를 출력 버퍼에 출력
    iFormatLength = kStrLen(pcFormatString);
    for(int i=0; i<iFormatLength; i++){
        if(pcFormatString[i] == '%'){
            i++;
            switch(pcFormatString[i]){
            case 's':// 문자열 출력
                pcCopyString = (char*)(va_arg(ap, char*));
                iCopyLength = kStrLen(pcCopyString);
                kMemCpy(pcBuffer+iBufferIndex, pcCopyString, iCopyLength);
                iBufferIndex += iCopyLength;
                break;

            case 'c':// 문자 출력
                pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
                iBufferIndex++;
                break;

            case 'd':// 정수 출력
            case 'i':
                iValue = (int)(va_arg(ap, int));
                iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
                break;

            case 'x':// 4바이트 hex 출력
            case 'X':
                qwValue = (dword)(va_arg(ap, dword)) & 0xffffffff;
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;
            
            case 'q':// 8바이트 hex 출력
            case 'Q':
            case 'p':
                qwValue = (qword)(va_arg(ap, qword));
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;
            
            default:// 그대로 출력
                pcBuffer[iBufferIndex] = pcFormatString[i];
                iBufferIndex++;
                break;
            }
        }
        else{// 일반 문자열 처리
            pcBuffer[iBufferIndex] = pcFormatString[i];
            iBufferIndex++;
        }
    }
    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}

// Tick Count를 반환
qword kGetTickCount(){
    return g_qwTickCount;
}