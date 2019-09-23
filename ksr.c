// ksr.c, 159

#include "spede.h"
#include "const-type.h"
#include "tools.h"
#include "ext-data.h"
#include "proc.h"
#include "ksr.h"

// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_que
void SpawnSR(func_p_t p)// arg: where process code starts
{
	int pid;

	KPANIC(QueEmpty(&avail_que),"Panic: out of PID!\n");

	pid = DeQue(&avail_que);
	Bzero((char*)&pcb[pid],sizeof(pcb_t));
	pcb[pid].state = READY;

	//if 'pid' is not IDLE, use a tool function to enqueue it to the ready queue
	if(pid != IDLE) EnQue(pid, &ready_que);

	//use a tool function to copy from 'p' to DRAM_START, for STACK_MAX bytes
	MemCpy((char *)(pid * STACK_MAX + DRAM_START), (char *)p, STACK_MAX);

	//create trapframe for process 'pid:'
	//1st position trapframe pointer in its PCB to the end of the stack
	pcb[pid].tf_p = (tf_t *)(DRAM_START + STACK_MAX - sizeof(tf_t));
	pcb[pid].tf_p -> efl = EF_DEFAULT_VALUE|EF_INTR;	// handle intr
	pcb[pid].tf_p -> cs = get_cs();						// duplicate from CPU
	pcb[pid].tf_p -> eip = DRAM_START;					// where code copied

}

// count run time and switch if hitting time limit
void TimerSR(void)
{
	int i;
	//1st notify PIC control register that timer event is now served
	outportb(PIC_CONT_REG, TIMER_SERVED_VAL);

	sys_time_count++;				//increment total up time
	pcb[run_pid].time_count++;		//increment process contiguous run time
	pcb[run_pid].total_time++;		//increment process total run time

	for (i = 0; i < PROC_MAX; i++) { //TODO: Fix awake from sleep
		if (pcb[i].state == RUN) {
			if (sys_time_count == pcb[i].total_time) {
				pcb[i].state = READY;
				EnQue(i, &ready_que);
			}
		}
	}

	if (run_pid == IDLE) return;
	if (pcb[run_pid].time_count==TIME_MAX)
	{
		EnQue(run_pid, &ready_que);
		pcb[run_pid].state = READY;
		pcb[run_pid].time_count = 0;
		run_pid = NONE;
	}
}

void SyscallSR(void) {
	switch (pcb[run_pid].tf_p->eax) {
		case SYS_GET_PID:
			pcb[run_pid].tf_p->ebx = run_pid;
			break;
		case SYS_GET_TIME:
			pcb[run_pid].tf_p->ebx = sys_time_count;
			break;
		case SYS_SLEEP:
			SysSleep();
			break;
		case SYS_WRITE:
			SysWrite();
			break;
		default:
			KPANIC_UCOND("Kernel Panic: no such syscall!\n");
	}
}

void SysSleep(void) {
	int sleep_sec = pcb[run_pid].tf_p->ebx;
	pcb[run_pid].wake_time = sys_time_count + sleep_sec * 100;
	pcb[run_pid].state = SLEEP;
	run_pid = NONE;
}

void SysWrite(void) {
	char *str = (char *) pcb[run_pid].tf_p->ebx;
	while (*str == '\0')
	{
		*sys_cursor = VGA_MASK_VAL || *str;
		//increment the cursor and string position
		str++;
		sys_cursor++;
		if (sys_cursor == VIDEO_END)
		{
			sys_cursor = VIDEO_START;
		}
	}
}
