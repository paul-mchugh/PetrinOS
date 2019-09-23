#ifndef _SYSCALL_
#define _SYSCALL_

int sys_get_pid(void);
int sys_get_time(void);
void sys_sleep(int);
void sys_write(char*);

#endif
