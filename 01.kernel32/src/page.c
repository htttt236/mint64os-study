#include "page.h"

void kInitializePageTables(){
    PML4ENTRY* pstPML4Entry;
    PDPTENTRY* pstPDPTEntry;
    PDENTRY* pstPDEntry;
    dword dwMappingAddress;
    int i;

    //PML4는 entry 1개
    pstPML4Entry = (PML4ENTRY*)0x100000; //1MB
    kSetPageEntryData(&(pstPML4Entry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
    for(i=1; i<PAGE_MAXENTRYCOUNT; i++){
        kSetPageEntryData(&(pstPML4Entry[i]), 0, 0, 0, 0);
    }

    //PDPT는 entry 64개
    pstPDPTEntry = (PDPTENTRY*)0x101000;
    for(i=0; i<64; i++){
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x102000+(i*PAGE_TABLESIZE), 
            PAGE_FLAGS_DEFAULT, 0);
    }
    for(i=64; i<PAGE_MAXENTRYCOUNT; i++){
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
    }

    //PD table 64개
    pstPDEntry = (PDENTRY*)0x102000;
    dwMappingAddress = 0;
    for(i=0; i<PAGE_MAXENTRYCOUNT*64; i++){
        kSetPageEntryData(&(pstPDEntry[i]), (i*(PAGE_DEFAULTSIZE>>20))>>12, 
            dwMappingAddress, PAGE_FLAGS_DEFAULT|PAGE_FLAGS_PS, 0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

void kSetPageEntryData(PTENTRY* pstEntry, dword dwUpperBaseAddress, 
    dword dwLowerBaseAddress, dword dwLowerFlags, dword dwUpperFlags){
    
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress|dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress&0xff)|dwUpperFlags;
}
