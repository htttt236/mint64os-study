#ifndef __ISR_H__
#define __ISR_H__

// 예외(Exception) 처리용 ISR
void kISRDivideError();
void kISRDebug();
void kISRNMI();
void kISRBreakPoint();
void kISROverflow();
void kISRBoundRangeExceeded();
void kISRInvalidOpcode();
void kISRDeviceNotAvailable();
void kISRDoubleFault();
void kISRCoprocessorSegmentOverrun();
void kISRInvalidTSS();
void kISRSegmentNotPresent();
void kISRStackSegmentFault();
void kISRGeneralProtection();
void kISRPageFault();
void kISR15();
void kISRFPUError();
void kISRAlignmentCheck();
void kISRMachineCheck();
void kISRSIMDError();
void kISRVirtualizationException();
void kISRControlProtectionException();
void kISRETCException();
void kISRHypervisorInjectionException();
void kISRVMMCommunicationException();
void kISRSecurityException();
void kISR31();

// 인터럽트(Interrupt) 처리용 ISR
void kISRTimer();
void kISRKeyboard();
void kISRSlavePIC();
void kISRSerial2();
void kISRSerial1();
void kISRParallel2();
void kISRFloppy();
void kISRParallel1();
void kISRRTC();
void kISRReserved();
void kISRNotUsed1();
void kISRNotUsed2();
void kISRMouse();
void kISRCoprocessor();
void kISRHDD1();
void kISRHDD2();
void kISRETCInterrupt();

#endif /*__ISR_H__*/