// proc.h, 159

#ifndef _PROC_
#define _PROC_

#include "syscall.h"

void Idle(void);
void Init(void);
void MyChildExitHandler(void);
void Login(void);

#endif
