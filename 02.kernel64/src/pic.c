#include "pic.h"
#include "assembly_utility.h"

void kInitializePIC(){
    // 마스터 PIC 컨트롤러 초기화
    kOutPortByte(PIC_MASTER_PORT1, 0x11);// ICW1, IC4비트(비트0) 1
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);// ICW2, 오프셋 32
    kOutPortByte(PIC_MASTER_PORT2, 0x04);// ICW3, 슬레이브 PIC 컨트롤러 위치 2
    kOutPortByte(PIC_MASTER_PORT2, 0x01);// ICW4, uPM 비트(비트0) 1

    // 슬레이브 PIC 컨트롤러 초기화
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);// ICW1, IC4비트(비트0) 1
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR+8);// ICW2, 오프셋 32
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);// ICW3, 슬레이브 PIC 컨트롤러 위치 2
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);// ICW4, uPM 비트(비트0) 1
}

void kMaskPICInterrupt(word wIRQBitmask){
    kOutPortByte(PIC_MASTER_PORT2, (byte)wIRQBitmask);// OCW1, IRQ 0-7
    kOutPortByte(PIC_SLAVE_PORT2,(byte)(wIRQBitmask>>8));// OCW1, IRQ 8-15
}

void kSendEOIToPIC(int iIRQNumber){
    kOutPortByte(PIC_MASTER_PORT1, 0x20);
    if(iIRQNumber >= 8){
        kOutPortByte(PIC_SLAVE_PORT1, 0x20);
    }
}