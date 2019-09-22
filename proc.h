// proc.h, 159

#ifndef _PROC_
#define _PROC_

#include "syscall.h"

void Idle(void);
void Init(void);
int sys_get_pid(void);
int sys_get_time(void);
void sys_write(char *);
void sys_sleep(int);

#endif
