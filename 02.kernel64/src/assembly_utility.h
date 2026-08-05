#ifndef __ASSEMBLY_UTILITY_H__
#define __ASSEMBLY_UTILITY_H__

#include "types.h"

byte kInPortByte(word wPort);
void kOutPortByte(word wPort, byte bData);
void kLoadGDTR(qword qwGDTRAdress);
void kLoadTR(word wTSSegmentOffset);
void kLoadIDTR(qword qwIDTRAddress);
void kEnableInterrupt();
void kDisableInterrupt();
qword kReadRFLAGS();

#endif /*__ASSEMBLY_UTILITY_H__*/