#include "pit.h"
#include "types.h"
#include "assembly_utility.h"


// PIT 초기화
void kInitializePIT(word wCount, bool bPeriodic){

    // PIT 컨트롤 레지스터(포트 0x43)로 카운터0 설정해 중지,
    // 한 번만 울리는 mode0으로 설정, LSB(하위8비트)rw 후 MSB(상위8비트)rw로 설정
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

    if(bPeriodic == true){// mode2로 변경
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    }

    // 카운터0(포트 0x40)에 LSB -> MSB 순으로 초기값 설정
    kOutPortByte(PIT_PORT_COUNTER0, wCount & 0xff);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

//카운터 0의 현재 값 반환
word kReadCounter0(){
    byte bHighByte, bLowByte;

    //  PIT 컨트롤 레지스터(포트 0x43)로 래치 커맨드 전송해서 카운터0 현재 값 읽기
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    // 카운터0(포트 0x40)에서 LSB -> MSB 순으로 값 읽기
    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);

    return ((bHighByte << 8) | bLowByte);
}

// 주어진 시간만큼 대기
// 함수 호출 시 PIT 컨트롤러 설정 바뀌므로, 이후 재설정 필요
// 정확한 측정을 위해 사용 전 인터럽트 비활성화 필요
// 약 50ms까지 측정 가능
void kWaitUsingDirectPIT(word wCount){
    word wLastCounter0;
    word wCurrentCounter0;

    // PIT 컨트롤러를 0~0xffff까지 반복해서 카운팅하도록 설정
    kInitializePIT(0, true);

    // 지금부터 wCount 이상 증가할 때까지 대기
    wLastCounter0 = kReadCounter0();
    while(true){
        // 현재 카운터0 값 반환
        wCurrentCounter0 = kReadCounter0();
        if(((wLastCounter0 - wCurrentCounter0) & 0xffff) >= wCount){
            break;
        }
    }
}