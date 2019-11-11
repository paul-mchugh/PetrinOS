// const-type.h, 159, needed constants & types

#ifndef _CONST_TYPE_			// to prevent name mangling recursion
#define _CONST_TYPE_			// to prevent name redefinition

#define TIMER_EVENT 32			// timer interrupt signal code
#define PIC_MASK_REG 0x21		// I/O loc # of PIC mask
#define PIC_MASK_VAL ~0x01		// mask code for PIC
#define PIC_CONT_REG 0x20		// I/O loc # of PIc control
#define TIMER_SERVED_VAL 0x60	// control code sent to PIC
#define VGA_MASK_VAL 0x0f00		// bold face, white on black

#define TIME_MAX 310			// max timer count, then rotate process
#define PROC_MAX 20				// max number of processes
#define STACK_MAX 4096			// process stack in bytes
#define QUE_MAX 20				// capacity of a process queue
#define STR_MAX 20
#define PAGE_MAX 100
#define PAGE_SIZE 4096
#define G1 0x40000000
#define G2 0x80000000

#define NONE -1					// to indicate none
#define IDLE 0					// Idle thread PID 0
#define DRAM_START 0xe00000		// 14 MB

#define SYSCALL_EVENT 128		// syscall event identifier code, phase2
#define SYS_GET_PID 129			// different types of syscalls
#define SYS_GET_TIME 130
#define SYS_SLEEP 131
#define SYS_WRITE 132
#define SYS_FORK 133
#define SYS_SET_CURSOR 134
#define SYS_GET_RAND 135
#define SYS_LOCK_MUTEX 136
#define SYS_UNLOCK_MUTEX 137
#define SYS_EXIT 138
#define SYS_WAIT 139
#define VIDEO_START ((unsigned short *)0xb8000)
#define VIDEO_END ((unsigned short *)0xb8000 + 25 * 80)
#define SYS_SIGNAL 140
#define SYS_KILL 141
#define SYS_READ 142
#define SYS_VFORK 143

#define SIGCHLD 17
#define SIGCONT 18

#define VIDEO_MUTEX 0

#define LOCKED 1
#define UNLOCKED 0

#define RO			0x00	//page flags
#define PRESENT		0x01
#define RW			0x02

typedef void (*func_p_t)(void);	// void-return function pointer type

typedef enum {AVAIL, READY, RUN, SLEEP, SUSPEND, WAIT, ZOMBIE, IO_WAIT} state_t;

typedef struct
{
	unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax, event, eip, cs, efl;
} tf_t;

typedef struct
{
	state_t state;
	tf_t *tf_p;
	unsigned int wake_time, time_count, total_time;	// pertaining to time.
	unsigned int ppid;
	unsigned int Dir;
	func_p_t signal_handler[32];
} pcb_t;

typedef struct
{
	int que[QUE_MAX];
	int tail;
} que_t;

typedef struct
{
	int lock;
	que_t suspend_que;
} mutex_t;

typedef struct
{
	que_t buffer;
	que_t wait_que;
} kb_t;

typedef struct
{
	int pid;
	union
	{
		unsigned int addr;
		char* content;
		unsigned int* entry;
	} u;
} page_t;

#endif							// to prevent name mangling

