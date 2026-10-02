#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "types.h"
#include "synchronization.h"
#include "harddisk.h"


// MINT 파일 시스템 시그니처(Signature)
#define FILESYSTEM_SIGNATURE                0x7E38CF10
// 클러스터의 크기(섹터 수), 4Kbyte
#define FILESYSTEM_SECTORSPERCLUSTER        8
// 파일 클러스터의 마지막 표시
#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF
// 빈 클러스터 표시
#define FILESYSTEM_FREECLUSTER              0x00
// 루트 디렉터리에 있는 최대 디렉터리 엔트리의 수
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ( ( FILESYSTEM_SECTORSPERCLUSTER * 512 ) / \
        sizeof( DIRECTORYENTRY ) )
// 클러스터의 크기(바이트 수)
#define FILESYSTEM_CLUSTERSIZE              ( FILESYSTEM_SECTORSPERCLUSTER * 512 )

// 파일 이름의 최대 길이
#define FILESYSTEM_MAXFILENAMELENGTH        24
// 하드 디스크 제어에 관련된 함수 포인터 타입 정의
typedef bool (* fReadHDDInformation ) ( bool bPrimary, bool bMaster,
        HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) ( bool bPrimary, bool bMaster, dword dwLBA,
        int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) ( bool bPrimary, bool bMaster, dword dwLBA,
        int iSectorCount, char* pcBuffer );



# pragma pack(push, 1)

//파티션 자료구조
typedef struct kPartitionStruct{
    // 부팅 가능 플래그. 0x80이면 부팅 가능, 0x00은 부팅 불가
    byte bBootableFlag;
    // 파티션의 시작 어드레스. 현재는 거의 사용하지 않으며 아래의 LBA 어드레스를 대신 사용
    byte vbStartingCHSAddress[3];
    // 파티션 타입
    byte bPartitionType;
    // 파티션의 마지막 어드레스. 현재는 거의 사용 안 함
    byte vbEndingCHSAddress[3];
    // 파티션의 시작 어드레스. LBA 어드레스로 나타낸 값
    dword dwStartingLBAAddress;
    // 파티션에 포함된 섹터 수
    dword dwSizeInSector;
}PARTITION;

// MBR 자료구조
typedef struct kMBRStruct{
    // 부트로더 코드가 위치하는 영역
    byte vbBootCode[430];

    // 파일 시스템 시그니처, 0x7e38cf10
    dword dwSignature;
    // 예약된 영역의 섹터 수
    dword dwReservedSectorCount;
    // 클러스터 링크 테이블 영역의 섹터 수
    dword dwClusterLinkSectorCount;
    // 클러스터의 전체 개수
    dword dwTotalClusterCount;

    // 파티션 테이블
    PARTITION vstPartition[4];

    // 부트 로더 시그니처, 0x55, 0xaa
    byte vbBootLoaderSignature[2];
}MBR;

// 디렉터리 엔트리 자료구조
typedef struct kDirectEntryStruct{
    // 파일 이름
    char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];
    // 파일의 실제 크기
    dword dwFileSize;
    // 파일이 시작하는 클러스터 인덱스
    dword dwStartClusterIndex;
}DIRECTORYENTRY;

#pragma pack(pop)

typedef struct kFileSystemManagerStruct{
    // 파일 시스템이 정상적으로 인식되었는지 여부
    bool bMounted;

    // 각 영역의 섹터 수와 시작 LBA 어드레스
    dword dwReservedSectorCount;
    dword dwClusterLinkAreaStartAddress;
    dword dwClusterLinkAreaSize;
    dword dwDataAreaStartAddress;
    // 데이터 영역의 클러스터의 총 개수
    dword dwTotalClusterCount;

    // 마지막으로 클러스터를 할당한 클러스터 링크 테이블의 섹터 오프셋을 저장
    dword dwLastAllocatedClusterLinkSectorOffset;

    // 파일 시스템 동기화 객체
    MUTEX stMutex;
}FILESYSTEMMANAGER;



bool kInitializeFileSystem();
bool kMount();
bool kFormat();
bool kGetHDDInformation(HDDINFORMATION* pstInformation);
bool kReadClusterLinkTable(dword dwOffset, byte* pbBuffer);
bool kWriteClusterLinkTable(dword dwOffset, byte* pbBuffer);
bool kReadCluster(dword dwOffset, byte* pbBuffer);
bool kWriteCluster(dword dwOffset, byte* pbBuffer);
dword kFindFreeCluster();
bool kSetClusterLinkData(dword dwClusterIndex, dword dwData);
bool kGetClusterLinkData(dword dwClusterIndex, dword* pdwData);
int kFindFreeDirectoryEntry();
bool kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
bool kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);



#endif /*__FILE_SYSTEM__*/