#ifndef __DEFINE_H__
#define __DEFINE_H__

#include "types.h"

// 조합에 사용할 매크로 (GDT)
#define GDT_TYPE_CODE           0x0A
#define GDT_TYPE_DATA           0x02
#define GDT_TYPE_TSS            0x09
#define GDT_FLAGS_LOWER_S       0x10
#define GDT_FLAGS_LOWER_DPL0    0x00
#define GDT_FLAGS_LOWER_DPL1    0x20
#define GDT_FLAGS_LOWER_DPL2    0x40
#define GDT_FLAGS_LOWER_DPL3    0x60
#define GDT_FLAGS_LOWER_P       0x80
#define GDT_FLAGS_UPPER_L       0x20
#define GDT_FLAGS_UPPER_DB      0x40
#define GDT_FLAGS_UPPER_G       0x80

// 실제로 사용할 매크로 (GDT)
#define GDT_FLAGS_LOWER_KERNELCODE \
    (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA \
    (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE \
    (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA \
    (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS \
    (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE \
    (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA \
    (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS \
    (GDT_FLAGS_UPPER_G)

// 세그먼트 디스크립터 오프셋 (GDT)
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// 기타 매크로 (GDT)
#define GDTR_STARTADDRESS   0x142000
#define GDT_MAXENTRY8COUNT  3
#define GDT_MAXENTRY16COUNT 1
#define GDT_TABLESIZE       ((sizeof(GDTENTRY8)*GDT_MAXENTRY8COUNT) + \
    sizeof(GDTENTRY16)*GDT_MAXENTRY16COUNT)
#define TSS_SEGMENTSIZE     (sizeof(TSSSEGMENT))

// 조합에 사용할 매크로 (IDT)
#define IDT_TYPE_INTERRUPT      0x0E
#define IDT_TYPE_TRAP           0x0F
#define IDT_FLAGS_DPL0          0x00
#define IDT_FLAGS_DPL1          0x20
#define IDT_FLAGS_DPL2          0x40
#define IDT_FLAGS_DPL3          0x60
#define IDT_FLAGS_P             0x80
#define IDT_FLAGS_IST0          0
#define IDT_FLAGS_IST1          1

// 실제로 사용할 매크로 (IDT)
#define IDT_FLAGS_KERNEL \
    (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER \
    (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// 기타 매크로 (IDT)
#define IDT_MAXENTRYCOUNT  100
#define IDTR_STARTADDRESS  (GDTR_STARTADDRESS + sizeof(GDTR) + \
    GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS   (IDTR_STARTADDRESS + sizeof(IDTR))
#define IDT_TABLESIZE      (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

#define IST_STARTADDRESS   0x700000
#define IST_SIZE           0x100000



#pragma pack(push, 1)

typedef struct kGDTRStruct{
    word wLimit;
    qword qwBaseAddress;
    word wPading;
    dword dwPading;
}GDTR, IDTR;

typedef struct kGDTEntry8Struct{
    word wLowerLimit;
    word wLowerBaseAddress;
    byte bUpperBaseAddress1;
    byte bTypeAndLowerFlag;// P, DPL(2비트), S, type(4비트)
    byte bUpperLimitAndUpperFlag;// G, D/B, L, AVL, lmit(4비트)
    byte bUpperBaseAddress2;
}GDTENTRY8;

typedef struct kGDTEntry16Struct{
    word wLowerLimit;
    word wLowerBaseAddress;
    byte bMiddleBaseAddress1;
    byte bTypeAndLowerFlag;// P, DPL(2비트), S, type(1, 0, B, 1)
    byte bUpperLimitAndUpperFlag;// G, 0, 0, AVL, limit(4비트)
    byte bMiddleBaseAddress2;
    dword dwUpperBaseAddress;
    dword dwReserved;
}GDTENTRY16;

typedef struct kTSSDataStruct{
    dword dwReserved1;
    qword qwRsp[3];
    qword qwReserved2;
    qword qwIST[7];
    qword qwReserved3;
    word wReserved;
    word wIOMapBaseAddress;
}TSSSEGMENT;

typedef struct kIDTEnryStruct{
    word wLowerBaseAddress;
    word wSegmentSelector;
    byte bIST;// 0(5비트), IST(3비트)
    byte bTypeAndFlags;// P, DPL(2비트), 0, type(4비트)
    word wMiddleBaseAddress;
    dword dwUpperBaseAddress;
    dword dwReserved;
}IDTENTRY;

#pragma pack(pop)

void kInitializeGDTTableAndTSS();
void kSetGDTEntry8(GDTENTRY8* pstEntry, dword dwBaseAddress, dword dwLimit, 
    byte bUpperFlags, byte bLowerFlags, byte bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, qword qwBaseAddress, dword dwLimit, 
    byte bUpperFlags, byte bLowerFlags, byte bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);
void kInitializeIDTTables();
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, word wSelector,
    byte bIST, byte bFlags, byte bType);
void kDummyHandler(void);

#endif /*__DEFINE_H__*/