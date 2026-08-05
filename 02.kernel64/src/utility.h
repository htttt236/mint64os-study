#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "types.h"

void kPrintString(int x, int y, const char* pcString);
void kMemSet(void* pvDestination, byte bData, int iSize);
int kMemCpy(void* pvDestination, const void* pvSource, int iSize);
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

#endif /*__UTILITY_H__*/