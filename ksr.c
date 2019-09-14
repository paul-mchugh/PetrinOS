// ksr.c, 159

#include "spede.h"
#include "const-type.h"
#include "ext-data.h"
#include "tools.h"
#include "proc.h"

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
	MemCpy((char *)DRAM_START, (char *)p, STACK_MAX);

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
	//1st notify PIC control register that timer event is now served
	outportb(PIC_CONT_REG, TIMER_SERVED_VAL);

	sys_time_count++;				//increment total up time
	pcb[run_pid].time_count++;		//increment process contiguous run time
	pcb[run_pid].total_time++;		//increment process total run time

	if (pcb[run_pid].time_count==TIME_MAX)
	{
		EnQue(run_pid, &ready_que);
		//TODO: check to make sure I did this right: //alter its state to indicate it is not running but ...
		pcb[run_pid].state = READY;
		pcb[run_pid].time_count = 0;
		run_pid = NONE;
	}
}

