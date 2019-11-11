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
void sys_exit(int exit_code);
int sys_wait(int *exit_code);
void sys_signal(int signal_name, func_p_t p);
void sys_kill(int pid, int signal_name);
void sys_read(char *str);
void sys_vfork(func_p_t p);

#endif
