// syscall.c
// system service calls for kernel services

#include "const-type.h"     // for SYS_GET_PID, etc., below
#include "syscall.h"
#include "proc.h"

int sys_get_pid(void) {     // phase2
   int pid;

   asm("movl %1, %%eax;     // # for kernel to identify service
	int $128;           // interrupt!
	movl %%ebx, %0"     // after, copy ebx to return
       : "=g" (pid)         // output from asm()
       : "g" (SYS_GET_PID)  // input to asm()
       : "eax", "ebx"       // clobbered registers
   );

   return pid;
}

int sys_get_time(void) {     // similar to sys_get_pid
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

void sys_sleep(int sleep_sec) {  // phase2
   asm("movl %0, %%eax;          // # for kernel to identify service
        movl %1, %%ebx;          // sleep seconds
        int $128"                // interrupt!
       :                         // no output from asm()
       : "g" (SYS_SLEEP), "g" (sleep_sec)  // 2 inputs to asm()
       : "eax", "ebx"            // clobbered registers
   );
}

void sys_write(char * str) {             // similar to sys_sleep
	asm("movl %0, %%eax;
	     movl %1, %%ebx;
	     int $128"
	    : 
	    : "g" (SYS_WRITE), "g" (str)
	    : "eax", "ebx"
	);
}
