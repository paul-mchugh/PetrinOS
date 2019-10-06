#ifndef _SYSCALL_
#define _SYSCALL_

int sys_get_pid(void);
int sys_get_time(void);
void sys_sleep(int);
void sys_write(char*);
void sys_set_cursor(int, int);
int sys_fork(void);
unsigned int sys_get_rand(void);
void sys_lock_mutex(int mutex_id);
void sys_unlock_mutex(int mutex_id);

#endif
