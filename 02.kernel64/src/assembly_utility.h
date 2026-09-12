#ifndef __ASSEMBLY_UTILITY_H__
#define __ASSEMBLY_UTILITY_H__

#include "types.h"
#include "task.h"

byte kInPortByte(word wPort);
void kOutPortByte(word wPort, byte bData);
void kLoadGDTR(qword qwGDTRAdress);
void kLoadTR(word wTSSegmentOffset);
void kLoadIDTR(qword qwIDTRAddress);
void kEnableInterrupt();
void kDisableInterrupt();
qword kReadRFLAGS();
qword kReadTSC();
void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);
void kHlt();
bool kTestAndSet(volatile byte* pbDestination, byte bCompare, byte bSource);
void kInitializeFPU();
void kSaveFPUContext(void* pvFPUContext);
void kLoadFPUContext(void* pvFPUContext);
void kSetTS();
void kClearTS();

#endif /*__ASSEMBLY_UTILITY_H__*/