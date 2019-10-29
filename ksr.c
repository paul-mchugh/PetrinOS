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
	pcb[pid].tf_p = (tf_t *)(DRAM_START + (STACK_MAX*(pid+1)) - sizeof(tf_t));
	pcb[pid].tf_p -> efl = EF_DEFAULT_VALUE|EF_INTR;	// handle intr
	pcb[pid].tf_p -> cs = get_cs();						// duplicate from CPU
	pcb[pid].tf_p -> eip = DRAM_START + STACK_MAX*pid;	// where code copied

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

	//iterate over the processes and if they are asleep and have passed
	//the wake time then wake them up
	for (i = 0; i < PROC_MAX; i++)
	{
		if (pcb[i].state==SLEEP && sys_time_count>=pcb[i].wake_time)
		{
			pcb[i].state = READY;
			EnQue(i, &ready_que);
		}
	}

	if (run_pid == IDLE) return;
	if (pcb[run_pid].time_count==TIME_MAX)
	{
		EnQue(run_pid, &ready_que);
		pcb[run_pid].state = READY;
		run_pid = NONE;
	}
}

void SyscallSR(void)
{
	switch (pcb[run_pid].tf_p->eax)
	{
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
		case SYS_SET_CURSOR:
			SysSetCursor();
			break;
		case SYS_FORK:
			SysFork();
			break;
		case SYS_GET_RAND:
			pcb[run_pid].tf_p->ebx = sys_rand_count;
			break;
		case SYS_LOCK_MUTEX:
			SysLockMutex();
			break;
		case SYS_UNLOCK_MUTEX:
			SysUnlockMutex();
			break;
		case SYS_EXIT:
			SysExit();
			break;
		case SYS_WAIT:
			SysWait();
			break;
		case SYS_SIGNAL:
			//SysSignal()
			break;
		case SYS_KILL:
			//SysKill()
			break;
		default:
			KPANIC_UCOND("Kernel Panic: no such syscall!\n");
	}
	if (run_pid != NONE)
	{
		pcb[run_pid].state = READY;
		EnQue(run_pid, &ready_que);
		run_pid = NONE;
	}
}

void AlterStack(int pid, func_p_t p)
{
	int* retEIP = &pcb[run_pid].tf_p->efl;	//store pointer to place where return ptr goes
	tf_t tmpTf = *pcb[run_pid].tf_p;		//copy original trapframe

	//lower the trapframe by 4 bytes
	pcb[run_pid].tf_p = (tf_t*)(((char*)pcb[run_pid].tf_p)-4);
	*pcb[run_pid].tf_p = tmpTf;

	//insert original EIP in gap & set eip to handler function
	*retEIP = pcb[run_pid].tf_p->eip;
	pcb[run_pid].tf_p->eip = p;
}

void SysSleep(void)
{
	int sleep_sec = pcb[run_pid].tf_p->ebx;
	pcb[run_pid].wake_time = sys_time_count + sleep_sec * 10;
	pcb[run_pid].state = SLEEP;
	run_pid = NONE;
}

void SysWrite(void)
{
	char *str = (char *) pcb[run_pid].tf_p->ebx;
	while (*str != '\0')
	{
		*sys_cursor = VGA_MASK_VAL | *str;
		//increment the cursor and string position
		str++;
		sys_cursor++;
		if (sys_cursor == VIDEO_END)
		{
			sys_cursor = VIDEO_START;
		}
	}
}

void SysSetCursor(void)
{
	// check later to make sure the screen is width 80, i think it is but i don't remember.
	sys_cursor = VIDEO_START + (80 * pcb[run_pid].tf_p->ebx) + pcb[run_pid].tf_p->ecx;
}

void SysFork(void)
{
	int pid;
	int offset; // from child to parent

	if(QueEmpty(&avail_que))
	{
		pcb[run_pid].tf_p->ebx = NONE;
		return;
	}
	pid = DeQue(&avail_que);
	EnQue(pid, &ready_que);

	// pcb copied and updated with new values
	MemCpy((char *)&pcb[pid],(char *)&pcb[run_pid], sizeof(pcb_t));
	pcb[pid].state = READY;
	pcb[pid].wake_time = 0;
	pcb[pid].time_count = 0;
	pcb[pid].ppid = run_pid;

	// copying parent's memory to child.
	MemCpy((char *)(pid * STACK_MAX + DRAM_START), (char *)(run_pid * STACK_MAX + DRAM_START), STACK_MAX);

	// calculate the byte distance between child and parent.
	offset = (pid - run_pid) * STACK_MAX;

	// though trapframe has been copied over, it's still pointing to parent, this fixes that
	pcb[pid].tf_p = (tf_t*)((char*)pcb[pid].tf_p + offset); // OS design is elegant
	// updating child trapframe
	pcb[pid].tf_p -> eip += offset;
	pcb[pid].tf_p -> ebp += offset;
	*((int*)pcb[pid].tf_p -> ebp) += offset;
	*(((int*)pcb[pid].tf_p -> ebp)+1) += offset;

	// setting the return values for sys_fork()
	pcb[run_pid].tf_p -> ebx = pid;
	pcb[pid].tf_p -> ebx = 0;	// how else will the child know it's the child?
}

void SysLockMutex(void)
{
	int mutex_id;

	mutex_id = pcb[run_pid].tf_p -> ebx;

	if (mutex_id == VIDEO_MUTEX)
	{
		if (video_mutex.lock == UNLOCKED)
		{
			video_mutex.lock = LOCKED;
		}
		else
		{
			EnQue(run_pid, &video_mutex.suspend_que);
			pcb[run_pid].state = SUSPEND;
			run_pid = NONE;
		}
	}
	else
	{
		KPANIC_UCOND("Panic: no such mutex ID!\n");
	}
}

void SysUnlockMutex(void)
{
	int mutex_id, released_pid;

	mutex_id = pcb[run_pid].tf_p -> ebx;

	if (mutex_id == VIDEO_MUTEX)
	{
		if (!QueEmpty(&video_mutex.suspend_que))
		{
			released_pid = DeQue(&video_mutex.suspend_que);
			pcb[released_pid].state = READY;
			EnQue(released_pid, &ready_que);
		}
		else
		{
			video_mutex.lock = UNLOCKED;
		}
	}
	else
	{
		KPANIC_UCOND("Panic: no such mutex ID!\n");
	}
}

void SysExit(void)
{
	int ppid = pcb[run_pid].ppid;
	int ec = pcb[run_pid].tf_p->ebx;
	if(pcb[ppid].state == WAIT)
	{
		//parent was waiting on us we need to wake it up and set its wait returns
		pcb[ppid].state = READY;
		EnQue(ppid, &ready_que);
		*((int*)pcb[ppid].tf_p->ebx) = ec;
		pcb[ppid].tf_p->ebx = run_pid; //exiting program
		//cleanup self
		pcb[run_pid].state = AVAIL;
		EnQue(run_pid, &avail_que);
	}
	else
	{
		//zombify self and wait for the parent
		pcb[run_pid].state = ZOMBIE;
	}
	run_pid = NONE;
}

void SysWait(void)
{
	int i = 0;
	int zpid = NONE;
	//look through the pcb for our zombie children
	for(i=0;i<PROC_MAX;i++)
	{
		if(pcb[i].state==ZOMBIE && pcb[i].ppid==run_pid)
		{
			zpid = i;
		}
	}

	if(zpid!=NONE)
	{
		//set the return registers
		*((int*)pcb[run_pid].tf_p->ebx) = pcb[zpid].tf_p->ebx; //exit code
		pcb[run_pid].tf_p->ebx = zpid; //exiting program
		//cleanup child
		pcb[zpid].state = AVAIL;
		EnQue(zpid, &avail_que);
	}
	else
	{
		//we need to wait for a child to terminate
		pcb[run_pid].state = WAIT;
		run_pid = NONE;
	}
}

void SysSignal(void)
{
	int sigNo = pcb[run_pid].tf_p->ebx;
	func_p_t svcPtr = (func_p_t)pcb[run_pid].tf_p->ecx;
	pcb[run_pid].signal_handler[sigNo] = svcPtr;
}

void SysKill(void)
{
	int tPid = pcb[run_pid].tf_p->ebx;
	int sigNo = pcb[run_pid].tf_p->ecx;
	int i;

	if(tPid==0&&sigNo==SIGCONT)
	{
		for(i=0;i<MAX_PID;i++)
		{
			if(pcb[i].ppid==run_pid&&pcb[i].state==SLEEP)
			{
				pcb[i].state = READY;
				EnQue(i, &ready_que);
			}
		}
	}
}
