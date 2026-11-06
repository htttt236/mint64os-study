#ifndef __MP_CONFIGURATION_TABLE_H__
#define __MP_CONFIGURATION_TABLE_H__

#include "types.h"


// MP 플로팅 포인터의 특성 바이트(Feature Byte)
#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE  0x00
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE     0x80

// 엔트리 타입(Entry Type)
#define MP_ENTRYTYPE_PROCESSOR                  0
#define MP_ENTRYTYPE_BUS                        1
#define MP_ENTRYTYPE_IOAPIC                     2
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT      3
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT   4

// 프로세서 CPU 플래그
#define MP_PROCESSOR_CPUFLAGS_ENABLE            0x01
#define MP_PROCESSOR_CPUFLAGS_BSP               0x02

// 버스 타입 스트링(Bus Type String)
#define MP_BUS_TYPESTRING_ISA                   "ISA"
#define MP_BUS_TYPESTRING_PCI                   "PCI"
#define MP_BUS_TYPESTRING_PCMCIA                "PCMCIA"
#define MP_BUS_TYPESTRING_VESALOCALBUS          "VL"

// 인터럽트 타입(Interrupt Type)
#define MP_INTERRUPTTYPE_INT                    0
#define MP_INTERRUPTTYPE_NMI                    1
#define MP_INTERRUPTTYPE_SMI                    2
#define MP_INTERRUPTTYPE_EXTINT                 3

// 인터럽트 플래그(Interrupt Flags)
#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY      0x00
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH           0x01
#define MP_INTERRUPT_FLAGS_ACTIVELOW            0x03
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER       0x00
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED        0x04
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED       0x0C



#pragma pack(push, 1)
// MP 플로팅 포인터 자료구조(MP Floating Pointer Data Structure)
typedef struct kMPFloatingPointStruct
{
    // 시그너처, _MP_
    char vcSignature[4];
    // MP 설정 테이블이 위치하는 어드레스
    dword dwMPConfigurationTableAddress;
    // MP 플로팅 포인터 자료구조의 길이, 16바이트
    byte bLength;
    // MultiProcessor Specification의 버전
    byte bRevision;
    // 체크섬
    byte bCheckSum;
    // MP 특성 바이트 1~5
    byte vbMPFeatureByte[5];
}MPFLOATINGPOINTER;

// MP 설정 테이블 헤더(MP Configuration Table Header) 자료구조
typedef struct kMPConfigurationTableHeaderStruct
{
    // 시그너처, PCMP
    char vcSignature[4];
    // 기본 테이블 길이
    word wBaseTableLength;
    // MultiProcessor Specification의 버전
    byte bRevision;
    // 체크섬
    byte bCheckSum;
    // OEM ID 문자열
    char vcOEMIDString[8];
    // PRODUCT ID 문자열
    char vcProductIDString[12];
    // OEM이 정의한 테이블의 어드레스
    dword dwOEMTablePointerAddress;
    // OEM이 정의한 테이블의 크기
    word wOEMTableSize;
    // 기본 MP 설정 테이블 엔트리의 개수
    word wEntryCount;
    // 로컬 APIC의 메모리 맵 I/O 어드레스
    dword dwMemoryMapIOAddressOfLocalAPIC;
    // 확장 테이블의 길이
    word wExtendedTableLength;
    // 확장 테이블의 체크섬
    byte bExtendedTableChecksum;
    // 예약됨
    byte bReserved;
}MPCONFIGURATIONTABLEHEADER;

// 프로세서 엔트리 자료구조(Processor Entry)
typedef struct kProcessorEntryStruct
{
    // 엔트리 타입 코드, 0
    byte bEntryType;
    // 프로세서에 포함된 로컬 APIC의 ID
    byte bLocalAPICID;
    // 로컬 APIC의 버전
    byte bLocalAPICVersion;
    // CPU 플래그
    byte bCPUFlags;
    // CPU 시그너처
    byte vbCPUSignature[4];
    // 특성 플래그
    dword dwFeatureFlags;
    // 예약됨
    dword vdwReserved[2];
}PROCESSORENTRY;

// 버스 엔트리 자료구조(Bus Entry)
typedef struct kBusEntryStruct
{
    // 엔트리 타입 코드, 1
    byte bEntryType;
    // 버스 ID
    byte bBusID;
    // 버스 타입 문자열
    char vcBusTypeString[6];
}BUSENTRY;

// I/O APIC 엔트리 자료구조(I/O APIC Entry)
typedef struct kIOAPICEntryStruct
{
    // 엔트리 타입 코드, 2
    byte bEntryType;
    // I/O APIC ID
    byte bIOAPICID;
    // I/O APIC 버전
    byte bIOAPICVersion;
    // I/O APIC 플래그
    byte bIOAPICFlags;
    // 메모리 맵 I/O 어드레스
    dword dwMemoryMapAddress;
}IOAPICENTRY;

// I/O 인터럽트 지정 엔트리 자료구조(I/O Interrupt Assignment Entry)
typedef struct kIOInterruptAssignmentEntryStruct
{
    // 엔트리 타입 코드, 3
    byte bEntryType;
    // 인터럽트 타입
    byte bInterruptType;
    // I/O 인터럽트 플래그
    word wInterruptFlags;
    // 발생한 버스 ID
    byte bSourceBUSID;
    // 발생한 버스 IRQ
    byte bSourceBUSIRQ;
    // 전달할 I/O APIC ID
    byte bDestinationIOAPICID;
    // 전달할 I/O APIC INTIN
    byte bDestinationIOAPICINTIN;
}IOINTERRUPTASSIGNMENTENTRY;

// 로컬 인터럽트 지정 엔트리 자료구조(Local Interrupt Assignment Entry)
typedef struct kLocalInterruptEntryStruct
{
    // 엔트리 타입 코드, 4
    byte bEntryType;
    // 인터럽트 타입
    byte bInterruptType;
    // 로컬 인터럽트 플래그
    word wInterruptFlags;
    // 발생한 버스 ID
    byte bSourceBUSID;
    // 발생한 버스 IRQ
    byte bSourceBUSIRQ;
    // 전달할 로컬 APIC ID
    byte bDestinationLocalAPICID;
    // 전달할 로컬 APIC INTIN
    byte bDestinationLocalAPICLINTIN;
}LOCALINTERRUPTASSIGNMENTENTRY;

#pragma pack(pop)

// MP 설정 테이블을 관리하는 자료구조
typedef struct kMPConfigurationManagerStruct
{
    // MP 플로팅 테이블
    MPFLOATINGPOINTER* pstMPFloatingPointer;
    // MP 설정 테이블 헤더
    MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;
    // 기본 MP 설정 테이블 엔트리의 시작 어드레스
    qword qwBaseEntryStartAddress;
    // 프로세서 또는 코어의 수
    int iProcessorCount;
    // PIC 모드 지원 여부
    bool bUsePICMode;
    // ISA 버스의 ID
    byte bISABusID;
} MPCONFIGURATIONMANAGER;


bool kFindMPFloatingPointerAddress(qword* pstAddress);
bool kAnalysisMPConfigurationTable();
MPCONFIGURATIONMANAGER* kGetMPConfigurationManager();
void kPrintMPConfigurationTable();
int kGetProcessorCount();


#endif /*__MP_CONFIGURATION_TABLE_H__*/