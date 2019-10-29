// syscall.c
// system service calls for kernel services

#include "const-type.h"		// for SYS_GET_PID, etc., below
#include "syscall.h"
#include "proc.h"

int sys_get_pid(void)		// phase2
{
	int pid;

	asm("movl %1, %%eax;		// # for kernel to identify service
		int $128;				// interrupt!
		movl %%ebx, %0"			// after, copy ebx to return
		: "=g" (pid)			// output from asm()
		: "g" (SYS_GET_PID)		// input to asm()
		: "eax", "ebx"			// clobbered registers
	);

	return pid;
}

int sys_get_time(void)		// similar to sys_get_pid
{
	int time;

	asm("movl %1, %%eax;
		int $128;
		movl %%ebx, %0"
		: "=g" (time)
		: "g" (SYS_GET_TIME)
		: "eax", "ebx"
	);
	return time;
}

void sys_sleep(int sleep_sec)	// phase2
{
	asm("movl %0, %%eax;			// # for kernel to identify service
		movl %1, %%ebx;			// sleep seconds
		int $128"				// interrupt!
		:						// no output from asm()
		: "g" (SYS_SLEEP), "g" (sleep_sec)	// 2 inputs to asm()
		: "eax", "ebx"			// clobbered registers
	);
}

void sys_write(char * str)		// similar to sys_sleep
{
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		int $128"
		:
		: "g" (SYS_WRITE), "g" (str)
		: "eax", "ebx"
	);
}

void sys_set_cursor(int row, int col)
{
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		movl %2, %%ecx;
		int $128"
		:
		: "g" (SYS_SET_CURSOR), "g" (row), "g" (col)
		: "eax", "ebx", "ecx"
	);
}

int sys_fork(void)
{
	int pid;
	asm("movl %1, %%eax;
		int $128;
		movl %%ebx, %0"
		: "=g" (pid)
		: "g" (SYS_FORK)
		: "eax", "ebx"
	);
	return pid;
}

unsigned int sys_get_rand(void)
{
	unsigned int rand;
	asm("movl %1, %%eax;
		int $128;
		movl %%ebx, %0"
		: "=g" (rand)
		: "g" (SYS_GET_RAND)
		: "eax", "ebx"
	);
	return rand;
}

void sys_lock_mutex(int mutex_id)
{
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		int $128"
		:
		: "g" (SYS_LOCK_MUTEX), "g" (mutex_id)
		: "eax", "ebx"
	);
}

void sys_unlock_mutex(int mutex_id)
{
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		int $128"
		:
		: "g" (SYS_UNLOCK_MUTEX), "g" (mutex_id)
		: "eax", "ebx"
	);
}

void sys_exit(int exit_code)
{
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		int $128;"
		:
		: "g" (SYS_EXIT), "g" (exit_code)
		: "eax", "ebx"
	);
}

int sys_wait(int *exit_code)
{
	int result;
	asm("movl %1, %%eax;
		movl %2, %%ebx;
		int $128;
		movl %%ebx, %0"
		: "=g" (result)
		: "g" (SYS_WAIT), "g" (exit_code)
		: "eax", "ebx"
	);
	return result;
}

void sys_signal(int signal_name, func_p_t p) {
	asm("movl %1, %%ebx;
		movl %2, %%ecx;
		movl %0, %%eax;
		int $128"
		:
		: "g" (SYS_SIGNAL), "g" (signal_name), "g" (p)
		: "eax", "ebx", "ecx"
	);
}

void sys_kill(int pid, int signal_name) {
	asm("movl %0, %%eax;
		movl %1, %%ebx;
		movl %2, %%ecx;
		int $128"
		:
		: "g" (SYS_KILL), "g" (pid), "g" (signal_name)
		: "eax", "ebx", "ecx"
	);
}
