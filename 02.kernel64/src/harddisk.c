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
        bStatus = kReadHDDAlternateStatus(bPrimary);

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
        bStatus = kReadHDDAlternateStatus(bPrimary);

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
    kSwapByteInWord(pstHDDInformation->vwModelNumber,
            sizeof(pstHDDInformation->vwModelNumber) / 2);
    kSwapByteInWord(pstHDDInformation->vwSerialNumber,
            sizeof(pstHDDInformation->vwSerialNumber) / 2);
    kSwapByteInWord(pstHDDInformation->vwFirmwareRevision,
            sizeof(pstHDDInformation->vwFirmwareRevision) / 2);
    

    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return true;
}

// word 내의 바이트 순서를 바꿈
static void kSwapByteInWord(word* pwData, int iWordCount){
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
    /*bool bWaitResult;*/

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
    //  DRQ polling 후, 데이터 수신
    //==========================================================================

    // 섹터 수만큼 루프를 돌면서 데이터 수신
    for(i=0; i<iSectorCount; i++){
        // BSY 해제 + DRQ 설정(안정 상태)이 될 때까지 대기
        if(kWaitForHDDDataRequest(bPrimary, &bStatus) == false){
            kPrintf("\n[HDD READ TIMEOUT] LBA=%X sector=%d/%d "
                    "status=%X primary=%d\n",
                    dwLBA + i, i + 1, iSectorCount, bStatus, bPrimary);
            // 동기화 처리
            kUnlock(&(gs_stHDDManager.stMutex));
            // 부분 전송이 성공으로 오인되지 않도록 0을 반환
            return 0;
        }

        // 한 섹터를 읽음
        for(j=0; j<512/2; j++){
            ((word*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
        }

        // 드라이브가 다음 블록을 준비할 시간을 줌
        kWaitHDD400ns(bPrimary);
    }

    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;

/*
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
*/
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
    /*bool bWaitResult;*/

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



    // 첫 번째 블록을 보낼 수 있는 상태가 될 때까지 대기
    if(kWaitForHDDDataRequest(bPrimary, &bStatus) == false){
        kPrintf("\n[HDD WRITE TIMEOUT] LBA=%X status=%X primary=%d\n",
                dwLBA, bStatus, bPrimary);
        // 동기화 처리
        kUnlock(&(gs_stHDDManager.stMutex));
        return 0;
    }

    //==========================================================================
    //  데이터 송신
    //==========================================================================

    // 섹터 수만큼 루프를 돌면서 데이터 송신
    for(i=0; i<iSectorCount; i++){
        // 한 섹터를 씀
        for(j=0; j<512/2; j++){
            kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((word*)pcBuffer)[lReadCount++]);
        }

        // 블록 전송이 끝난 뒤 드라이브가 상태를 갱신할 시간을 줌
        kWaitHDD400ns(bPrimary);

        // 다음 블록을 보낼 수 있는 상태(BSY 해제 + DRQ 설정)가 될 때까지 대기
        if(i < iSectorCount - 1){
            if(kWaitForHDDDataRequest(bPrimary, &bStatus) == false){
                kPrintf("\n[HDD WRITE TIMEOUT] LBA=%X sector=%d/%d "
                        "status=%X primary=%d\n",
                        dwLBA + i, i + 1, iSectorCount, bStatus, bPrimary);
                // 동기화 처리
                kUnlock(&(gs_stHDDManager.stMutex));
                return 0;
            }
        }
        // 마지막 블록이면 명령 완료(BSY 해제)까지 대기하고 결과를 확인
        else{
            if(kWaitForHDDNoBusy(bPrimary) == false){
                kPrintf("\n[HDD WRITE BUSY TIMEOUT] LBA=%X primary=%d\n",
                        dwLBA, bPrimary);
                // 동기화 처리
                kUnlock(&(gs_stHDDManager.stMutex));
                return 0;
            }

            bStatus = kReadHDDAlternateStatus(bPrimary);
            if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
                kPrintf("\n[HDD WRITE ERROR] LBA=%X status=%X\n",
                        dwLBA, bStatus);
                // 동기화 처리
                kUnlock(&(gs_stHDDManager.stMutex));
                return 0;
            }
        }
    }

    // 동기화 처리
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
/*
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
*/
}

// 인터럽트를 acknowledge하지 않는 Alternate Status 레지스터를 읽음
// 일반 Status 레지스터(0x1f7/0x177)는 읽는 순간 보류 중인 IRQ가 지워지므로
// 상태 확인에는 이 함수를 사용해야 함
static byte kReadHDDAlternateStatus(bool bPrimary){
    if(bPrimary == true){
        return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT);
    }
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT);
}

// BSY가 해제되고 DRQ가 설정될 때까지 일정 시간 대기
// 이 조건은 드라이브가 준비될 때까지 유지되는 안정 상태이므로
// polling으로 확실하게 검출할 수 있음
static bool kWaitForHDDDataRequest(bool bPrimary, byte* pbStatus){
    qword qwStartTickCount;
    byte bStatus;

    qwStartTickCount = kGetTickCount();

    while(true){
        bStatus = kReadHDDAlternateStatus(bPrimary);

        // 에러가 발생하면 즉시 종료
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
            *pbStatus = bStatus;
            return false;
        }

        // busy가 아니고 데이터 요청이 설정되어 있으면 전송 가능
        if(((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) &&
            ((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST)){
            *pbStatus = bStatus;
            return true;
        }

        if((kGetTickCount() - qwStartTickCount) > HDD_WAITTIME){
            *pbStatus = bStatus;
            return false;
        }

        //kSleep(1);
    }
}

// Alternate Status를 여러 번 읽어 약 400ns 동안 대기
// 블록 전송 직후 드라이브가 다음 블록의 상태를 준비할 시간을 줌
static void kWaitHDD400ns(bool bPrimary){
    int i;

    for(i=0; i<15; i++){
        kReadHDDAlternateStatus(bPrimary);
    }
}