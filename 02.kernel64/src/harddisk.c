#include "harddisk.h"
#include "assembly_utility.h"
#include "utility.h"
#include "console.h"


// 하드 디스크를 관리하는 자료구조
static HDDMANAGER gs_stHDDManager;

// 하드 디스크 디바이스 드라이버 초기화
bool kInitializeHDD(){
    // 뮤텍스 초기화
    kInitializeMutex(&(gs_stHDDManager.stMutex));

    // 인터럽트 플래그 초기화
    gs_stHDDManager.bPrimaryInterruptOccur = false;
    gs_stHDDManager.bSecondaryInterruptOccur = false;

    // 첫 번째와 두 번째 PATA 포트의 디지털 출력 레지스터(포트 0x3f6와 0x376)에 0을
    // 출력하여 하드 디스크 컨트롤러의 인터럽트를 활성화
    kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

    // 하드 디스크 정보 요청
    if(kReadHDDInformation(true, false, &(gs_stHDDManager.stHDDInformation)) == false){
        gs_stHDDManager.bHDDDetected = false;
        gs_stHDDManager.bCanWrite = false;
        return false;
    }

    // 하드 디스크가 검색되었으면 QEMU에서만 쓸 수 있도록 설정
    gs_stHDDManager.bHDDDetected = true;
    if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0){
        gs_stHDDManager.bCanWrite = true;
    }
    else{
        gs_stHDDManager.bCanWrite = false;
    }
    return true;
}

// 하드 디스크의 상태를 반환
static byte kReadHDDStatus(bool bPrimary){
    if(bPrimary == true){
        // 첫 번째 PATA 포트의 상태 레지스터(포트 0x1f7)에서 값을 반환
        return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
    }
    // 두 번째 PATA 포트의 상태 레지스터(포트 0x177)에서 값을 반환
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

// 하드 디스크의 busy가 해제될 때까지 일정 시간 대기
static bool kWaitForHDDNoBusy(bool bPrimary){
    qword qwStartTickCount;
    byte bStatus;

    // 대기를 시작한 시간을 저장
    qwStartTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크의 busy가 헤재될 때까지 대기
    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
        // HDD의 상태를 반환
        bStatus = kReadHDDStatus(bPrimary);

        // busy 비트(비트 7)이 설정되어 있지 않으면 busy가 해제된 것이므로 종료
        if((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY){
            return true;
        }
        kSleep(1);
    }
    return false;
}

// 하드 디스크가 ready될 때까지 일정 시간 대기
static bool kWaitForHDDReady(bool bPrimary){
    qword qwStartTickCount;
    byte bStatus;

    // 대기를 시작한 시간을 저장
    qwStartTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크가 ready가 될 때까지 대기
    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
        // HDD의 상태를 반환
        bStatus = kReadHDDStatus(bPrimary);

        // ready 비트(비트 6)이 설정되어 있으면 데이터를 받을 준비가 된 것이므로 종료
        if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY){
            return true;
        }
        kSleep(1);
    }
    return false;
}

// 인터럽트 발생 여부를 설정
void kSetHDDInterruptFlag(bool bPrimary, bool bFlag){
    if(bPrimary == true){
        gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
    }
    else{
        gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
    }
}

// 인터럽트가 발생할 때까지 대기
static bool kWaitForHDDInterrupt(bool bPrimary){
    qword qwTickCount;

    // 대기를 시작한 시간을 저장
    qwTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크의 인터럽트가 발생할 때까지 대기
    while(kGetTickCount() - qwTickCount <= HDD_WAITTIME){
        // 하드 디스크 자료구조에 인터럽트 발생 플래그를 확인
        if((bPrimary == true) && (gs_stHDDManager.bPrimaryInterruptOccur == true)){
            return true;
        }
        else if((bPrimary == false) && (gs_stHDDManager.bSecondaryInterruptOccur == true)){
            return true;
        }
    }
    return false;
}

// 하드 디스크의 정보를 읽음
bool kReadHDDInformation(bool bPrimary, bool bMaster, HDDINFORMATION* pstHDDInformation){
    word wPortBase;
    qword qwLastTickCount;
    byte bStatus;
    byte bDriverFlag;
    int i;
    word wTemp;
    bool bWaitResult;

    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    if(bPrimary == true){
        // 첫 번째 PATA 포트이면 포트 0x1f0을 저장
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else{
        // 두 번째 PATA 포트이면 포트 0x170을 저장
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 동기화 처리
    kLock(&(gs_stHDDManager.stMutex));

    // 아직 수행 중인 커맨드가 있다면 일정 시간 끝날 때까지 대기
    if(kWaitForHDDNoBusy(bPrimary) == false){
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    //==========================================================================
    //  LBA 어드레스와 드라이브 및 헤드에 관련된 레지스터 설정
    //      드라이브와 헤드 정보만 있으면 됨
    //==========================================================================

    // 드라이브와 헤드 데이터 설정
    if(bMaster == true){
        // 마스터이면 LBA 플래그만 설정
        bDriverFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else{
        // 슬레이브이면 LBA 플래그에 슬레이브 플래그도 같이 설정
        bDriverFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // 드라이브/헤드 레지스터 (포트 0x1f6 또는 0x176)에 설정된 값을 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriverFlag);

    //==========================================================================
    //  커맨드 전송 후, 인터럽트 대기
    //==========================================================================

    // 커맨드를 받아들일 준비가 될 때까지 일정 시간 동안 대기
    if(kWaitForHDDReady(bPrimary) == false){
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    // 인터럽트 플래그를 초기화
    kSetHDDInterruptFlag(bPrimary, false);

    // 커맨드 레지스터(포트 0x1f7 또는 0x177)에 드라이브 인식 커맨드(0xec)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

    // 처리가 완료될 때까지 인터럽트 발생을 기다림
    bWaitResult = kWaitForHDDInterrupt(bPrimary);
    // 에러가 발생하거나 인터럽트가 발생하지 않으면 문제가 발생한 것이므로 종료
    bStatus = kReadHDDStatus(bPrimary);
    if((bWaitResult == false) || ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)){
        //동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    //==========================================================================
    //  데이터 수신
    //==========================================================================

    // 한 섹터를 읽음
    for(i=0; i<512/2; i++){
        ((word*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }

    // 문자열은 바이트 순서로 다시 반환
    kSwqpByteInWord(pstHDDInformation->vwModelNumber,
            sizeof(pstHDDInformation->vwModelNumber) / 2);
    kSwqpByteInWord(pstHDDInformation->vwSerialNumber,
            sizeof(pstHDDInformation->vwSerialNumber) / 2);
    kSwqpByteInWord(pstHDDInformation->vwFirmwareRevision,
            sizeof(pstHDDInformation->vwFirmwareRevision) / 2);
    

    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return true;
}

// word 내의 바이트 순서를 바꿈
static void kSwqpByteInWord(word* pwData, int iWordCount){
    int i;
    word wTemp;

    for(i=0; i<iWordCount; i++){
        wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}

// 하드 디스크의 섹터를 읽음
// 최대 256개의 섹터를 읽을 수 있음, 실제로 읽은 섹터 수를 반환
int kReadHDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount, char* pcBuffer){
    word wPortBase;
    int i, j;
    byte bDriveFlag;
    byte bStatus;
    long lReadCount = 0;
    bool bWaitResult;

    // 범위 검사. 최대 256 섹터를 처리할 수 있음
    if((gs_stHDDManager.bHDDDetected == false) ||
        (iSectorCount <= 0) || (256 < iSectorCount) ||
        ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))
    {
        return 0;
    }

    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    if(bPrimary == true){
        // 첫 번째 PATA 포트이면 포트 0x1f0 저장
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else{
        // 두 번째 PATA 포트이면 포트 0x170을 저장
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 동기화 처리
    kLock(&(gs_stHDDManager.stMutex));

    // 아직 수행 중인 커맨드가 있다면 일정 시간 동안 끝날 때까지 대기
    if(kWaitForHDDNoBusy(bPrimary) == false){
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    //==========================================================================
    //  데이터 레지스터 설정
    //      LBA 모드의 경우, 섹터 번호 -> 실린더 번호 -> 헤드 번호의 순으로 
    //      LBA 어드레스를 대입
    //==========================================================================

    // 섹터 수 레지스터(포트 0x1f2 또는 0x172)에 쓸 섹터 수를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    // 섹터 번호 레지스터(포트 0x1f3 또는 0x173)에 쓸 섹터 위치(LBA 0~7비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터(포트 0x1f4 또는 0x174)에 쓸 섹터 위치(LBA 8~15비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터(포트 0x1f5 또는 0x175)에 쓸 섹터 위치(LBA 16~23비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // 드라이브와 헤드 데이터 설정
    if(bMaster == true){
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else{
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // 드라이브/헤드 레지스터(포트 0x1f6 또는 0x176)에 쓸 섹터의 위치(LBA 24~27비트)와
    // 설정된 값을 같이 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0f));

    //==========================================================================
    //  커맨드 전송
    //==========================================================================

    // 커맨드를 받아들일 준비가 될 때까지 일정 시간 동안 대기
    if(kWaitForHDDReady(bPrimary) == false){
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    // 인터럽트 플래그를 초기화
    kSetHDDInterruptFlag(bPrimary, false);

    // 커맨드 레지스터 (포트 0x1f7 또는 0x177)에 읽기(0x20)을 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

    //==========================================================================
    //  인터럽트 대기 후, 데이터 수신
    //==========================================================================

    // 섹터 수만큼 루프를 돌면서 데이터 수신
    for(i=0; i<iSectorCount; i++){
        // 에러가 발생하면 종료
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
            kPrintf("error occur\n");
            // 동기화 처리
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 수신되길 기다림
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){
            // 처리가 완료될 때까지 일정 시간 동안 인터럽트를 기다림
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, false);
            // 인터럽트가 발생하지 않으면 문제가 발생한 것이므로 종료
            if(bWaitResult == false){
                kPrintf("interrupt not occur\n");
                // 동기화 처리
                kUnlock(&(gs_stHDDManager.stMutex));
                return false;
            }
        }

        // 한 섹터를 읽음
        for(j=0; j<512/2; j++){
            ((word*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
        }
    }
    
    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}

// 하드 디스크에 섹터를 씁
// 최대 256개 섹터 쓸 수 있음, 실제로 쓴 섹터 수 반환
int kWriteHDDSector(bool bPrimary, bool bMaster, dword dwLBA, int iSectorCount, char* pcBuffer){
    word wPortBase;
    word wTemp;
    int i, j;
    byte bDriveFlag;
    byte bStatus;
    long lReadCount = 0;
    bool bWaitResult;

    // 범위 검사. 최대 256섹터를 처리할 수 있음
    if((gs_stHDDManager.bCanWrite == false) ||
        (iSectorCount <= 0) || (256 < iSectorCount) ||
        ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))
    {
        return 0;
    }

    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    if(bPrimary == true){
        // 첫 번째 PATA 포트이면 포트 0x1f0을 저장
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else{
        // 두 번째 PATA 포트이면 포트 0x170을 저장
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 아직 수행 중인 커맨드가 있다면 일정 시간 동안 끝날 때까지 대기
    if(kWaitForHDDNoBusy(bPrimary) == false){
        return false;
    }

    // 동기화 처리
    kLock(&(gs_stHDDManager.stMutex));

    //==========================================================================
    //  데이터 레지스터 설정
    //      LBA 모드의 경우, 섹터 번호 -> 실린더 번호 -> 헤드 번호의 순으로 
    //      LBA 어드레스를 대입
    //==========================================================================

    // 섹터 수 레지스터(포트 0x1f2 또는 0x172)에 쓸 섹터 수를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    // 섹터 번호 레지스터(포트 0x1f3 또는 0x173)에 쓸 섹터 위치(LBA 0~7비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터(포트 0x1f4 또는 0x174)에 쓸 섹터 위치(LBA 8~15비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터(포트 0x1f5 또는 0x175)에 쓸 섹터 위치(LBA 16~23비트)를 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // 드라이브와 헤드 데이터 설정
    if(bMaster == true){
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else{
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // 드라이브/헤드 레지스터(포트 0x1f6 또는 0x176)에 쓸 섹터의 위치(LBA 24~27비트)와
    // 설정된 값을 같이 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0f));

    //==========================================================================
    //  커맨드 전송 후, 데이터 송신이 가능할 때까지 대기
    //==========================================================================

    // 커맨드를 받아들일 준비가 될 때까지 일정 시간 동안 대기
    if(kWaitForHDDReady(bPrimary) == false){
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return false;
    }

    // 커맨드 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

    // 데이터 송신이 가능할 때까지 대기
    while(true){
        bStatus = kReadHDDStatus(bPrimary);
        // 에러가 발생하면 종료
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
            // 동기화 처리
            kUnlock(&(gs_stHDDManager.stMutex));
            return 0;
        }

        // Data Request 비트가 설정되었다면 데이터 송신 가능
        if((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST){
            break;
        }

        kSleep(1);
    }

    //==========================================================================
    //  데이터 송신 후, 인터럽트 대기
    //==========================================================================

    // 섹터 수만큼 루프를 돌면서 데이터 송신
    for(i=0; i<iSectorCount; i++){
        // 인터럽트 플래그를 초기화하고 한 섹터를 씀
        kSetHDDInterruptFlag(bPrimary, false);
        for(j=0; j<512/2; j++){
            kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((word*)pcBuffer)[lReadCount++]);
        }

        // 에러가 발생하면 종료
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
            // 동기화 처리
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 처리가 완료되길 기다림
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){
            // 처리가 완료될 때까지 일정 시간 동안 인터럽트를 기다림
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, false);
            // 인터럽트가 발생하지 않으면 문제가 발생한 것이므로 종료
            if(bWaitResult == false){
                // 동기화 처리
                kUnlock(&(gs_stHDDManager.stMutex));
                return false;
            }
        }
    }

    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}