#ifndef __INTERRUPT_HANDLER_H__
#define __INTERRUPT_HANDLER_H__

#include "types.h"

void kCommonExceptionHandler(int iVectorNumber, qword qwErrorcode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);

#endif /*__INTERRUPT_HANDLER_H__*/