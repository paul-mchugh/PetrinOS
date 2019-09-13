// ext-data.h, 159
// kernel data are all declared in kernel.c during bootstrap
// .c code, if needed these, include this

#ifndef _EXT_DATA_
#define _EXT_DATA_

#include "spede.h"
#include "const-type.h"


extern int run_pid;					// PID of current selected running process
extern que_t avail_que, ready_que;	// avail PID and those created/ready to run
extern pcb_t pcb[PROC_MAX];			// Process Control Blocks
extern unsigned int sys_time_count;	// total time the system has been up
extern struct i386_gate *idt;		// interrupt descriptor table

#endif
