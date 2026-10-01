#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "types.h"
#include "synchronization.h"


// 첫 번째 PATA 포트(Primary PATA Port)와 두 번째 PATA 포트(Secondary PATA Port)의 정보
#define HDD_PORT_PRIMARYBASE                0x1F0
#define HDD_PORT_SECONDARYBASE              0x170

// 포트 인덱스에 관련된 매크로
#define HDD_PORT_INDEX_DATA                 0x00
#define HDD_PORT_INDEX_SECTORCOUNT          0x02
#define HDD_PORT_INDEX_SECTORNUMBER         0x03
#define HDD_PORT_INDEX_CYLINDERLSB          0x04
#define HDD_PORT_INDEX_CYLINDERMSB          0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD         0x06
#define HDD_PORT_INDEX_STATUS               0x07
#define HDD_PORT_INDEX_COMMAND              0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT        0x206

// 커맨드 레지스터에 관련된 매크로
#define HDD_COMMAND_READ                    0x20
#define HDD_COMMAND_WRITE                   0x30
#define HDD_COMMAND_IDENTIFY                0xEC

// 상태 레지스터에 관련된 매크로
#define HDD_STATUS_ERROR                    0x01
#define HDD_STATUS_INDEX                    0x02
#define HDD_STATUS_CORRECTEDDATA            0x04
#define HDD_STATUS_DATAREQUEST              0x08
#define HDD_STATUS_SEEKCOMPLETE             0x10
#define HDD_STATUS_WRITEFAULT               0x20
#define HDD_STATUS_READY                    0x40
#define HDD_STATUS_BUSY                     0x80

// 드라이브/헤드 레지스터에 관련된 매크로
#define HDD_DRIVEANDHEAD_LBA                0xE0
#define HDD_DRIVEANDHEAD_SLAVE              0x10

// 디지털 출력 레지스터에 관련된 매크로
#define HDD_DIGITALOUTPUT_RESET             0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT  0x01

// 하드 디스크의 응답을 대기하는 시간(millisecond)
#define HDD_WAITTIME                        500
// 한번에 HDD에 읽거나 쓸 수 있는 섹터의 수
#define HDD_MAXBULKSECTORCOUNT              256

#pragma pack(push, 1)

typedef struct kHDDInformationStruct{
    // 설정값
    word wConfiguration;

    // 실린더 수
    word wNumberOfCylinder;
    word wReserved1;

    // 헤드 수
    word wNumberOfHead;
    word wUnformattedBytesPerTrack;
    word wUnformattedBytesPerSector;

    // 실린더당 섹터 수
    // 실린더당 섹터 수
    word wNumberOfSectorPerCylinder;
    word wInterSectorGap;
    word wBytesInPhaseLock;
    word wNumberOfVendorUniqueStatusword;
    
    // 하드 디스크의 시리얼 넘버
    word vwSerialNumber[10];
    word wControllerType;
    word wBufferSize; 
    word wNumberOfECCBytes;
    word vwFirmwareRevision[4];
    
    // 하드 디스크의 모델 번호
    word vwModelNumber[20];
    word vwReserved2[13];
    
    // 디스크의 총 섹터 수
    dword dwTotalSectors;     
    word vwReserved3[196];
}HDDINFORMATION;

// 하드 디스크를 관리하는 구조체
typedef struct kHDDManagerStruct{
    // HDD 존재 여부와 쓰기를 수행할 수 있는지 여부
    bool bHDDDetected;
    bool bCanWrite;

    // 인터럽트 발생 여부와 동기화 객체
    volatile bool bPrimaryInterruptOccur;
    volatile bool bSecondaryInterruptOccur;
    MUTEX stMutex;

    // HDD 정보
    HDDINFORMATION stHDDInformation;
}HDDMANAGER;


bool kInitializeHDD();
static byte kReadHDDStatus(bool bPrimary);
static bool kWaitForHDDNoBusy(bool bPrimary);
static bool kWaitForHDDReady(bool bPrimary);
void kSetHDDInterruptFlag(bool bPrimary, bool bFlag);
static bool kWaitForHDDInterrupt(bool bPrimary);
bool kReadHDDInformation(bool bPrimary, bool bMaster, HDDINFORMATION* pstHDDInformation);
static void kSwqpByteInWord(word* pwData, int iWordCount);
int kReadHDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount, char* pcBuffer);
int kWriteHDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount, char* pcBuffer);


#endif /*__HARDDISK_H__*/