// ksr.h, 159

#ifndef _KSR_
#define _KSR_

#include "const-type.h"

void SpawnSR(func_p_t p);
void TimerSR(void);
void SyscallSR(void);
void AlterStack(int pid, func_p_t p);
void KBSR(void);
void TTYSR(void);
void TTYdspSR(void);
void TTYkbSR(void);
void SysSleep(void);
void SysWrite(void);
void SysSetCursor(void);
void SysFork(void);
void SysLockMutex(void);
void SysUnlockMutex(void);
void SysExit(void);
void SysWait(void);
void SysSignal(void);
void SysKill(void);
void SysRead(void);
void SysVfork(void);

#endif
