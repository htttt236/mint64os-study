#ifndef __MODE_SWITCH_H__
#define __MODE_SWITCH_H__

#include "types.h"

void kReadCPUID(dword dwEAX, dword* pdwEAX, dword* pdwEBX, dword* pdwECX, dword* pdwEDX);
void kSwitchAndExecute64bitKernel();

#endif /*__MODE_SWITCH_H__*/