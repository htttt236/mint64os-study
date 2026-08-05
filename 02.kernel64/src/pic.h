#ifndef __PIC_H__
#define __PIC_H__

#include "types.h"

#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0X21
#define PIC_SLAVE_PORT1 0Xa0
#define PIC_SLAVE_PORT2 0xa1

#define PIC_IRQSTARTVECTOR 0x20// IDT 테이블에서 인터럽트 벡터 시작위치

void kInitializePIC();
void kMaskPICInterrupt(word wIRQBitmask);
void kSendEOIToPIC(int iIRQNumber);

#endif /*__PIC_H__*/