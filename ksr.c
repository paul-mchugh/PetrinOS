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
	unsigned term;

	KPANIC(QueEmpty(&avail_que),"Panic: out of PID!\n");

	pid = DeQue(&avail_que);
	Bzero((char*)&pcb[pid],sizeof(pcb_t));
	pcb[pid].state = READY;

	// handling STDIN/STDOUT
        term = (pid==IDLE) ? CONSOLE : TTY;
	pcb[pid].STDOUT = term;
	pcb[pid].STDIN = term;
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
	pcb[pid].Dir = KDir; //Kernel initialized & forked process are run in real memory
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
			SysSignal();
			break;
		case SYS_KILL:
			SysKill();
			break;
		case SYS_READ:
			SysRead();
			break;
		case SYS_VFORK:
			SysVfork();
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

	//switch to Kernel VM
	set_cr3(KDir);
}

void AlterStack(int pid, func_p_t p)
{
	int* retEIP = &pcb[pid].tf_p->efl;	//store pointer to place where return ptr goes

	set_cr3(pcb[pid].Dir);		//change address space to that of the target pid

	//lower the trapframe by 4 bytes
	MemCpy(((char*)pcb[pid].tf_p)-4, (char*)pcb[pid].tf_p, sizeof(tf_t));
	pcb[pid].tf_p = (tf_t*)(((char*)pcb[pid].tf_p)-4);

	//insert original EIP in gap & set eip to handler function
	*retEIP = pcb[pid].tf_p->eip;
	pcb[pid].tf_p->eip = (int)p;
	set_cr3(pcb[run_pid].Dir);	//change address space to the callers pid
}

void KBSR(void)
{
	char nc;
	int pid;
	if (!cons_kbhit())	// if nothing, return
		return;
	nc = cons_getchar();
	if (nc == '$')
		breakpoint();
	if (QueEmpty(&kb.wait_que))
	{
		EnQue((int)nc, &kb.buffer);
	}
	else
	{
		pid = DeQue(&kb.wait_que);
		pcb[pid].state = READY;
		set_cr3(pcb[pid].Dir);		//set the address space to the receiving process
		pcb[pid].tf_p->ebx = (int)nc;
		EnQue(pid, &ready_que);
	}
}

void TTYdspSR()
{
	int pid, i;
	unsigned int cr3Copy;
	//outportb(PIC_CONT_REG, TTY_SERVED_VAL);
	if (!QueEmpty(&tty.echo)) {
		char out;
		out = DeQue(&tty.echo);
		outportb(tty.port, out);
		return;
    }
    if (QueEmpty(&tty.dsp_wait_que))
            return;
	pid = tty.dsp_wait_que.que[0];	// reading, not DeQue-ing
	// (virtual memory switching, in order to use string addr)
	cr3Copy = get_cr3();
	set_cr3(pcb[pid].Dir);	// switching address space to waiting process.
	if (*tty.dsp_str != '\0')
	{
		if(*tty.dsp_str!='\r')
		{
			outportb(tty.port, *tty.dsp_str);
		}
		else
		{
			outportb(tty.port, '\r');
		}
		tty.dsp_str++;
	}
	else
	{
		pid = DeQue(&tty.dsp_wait_que); // already set but who cares
		pcb[pid].state = READY;
		EnQue(pid, &ready_que);
	}
}

void TTYSR() {
	char status;
	outportb(PIC_CONT_REG, TTY_SERVED_VAL);
	status = inportb(tty.port+IIR);
	if (status == IIR_TXRDY) {
		TTYdspSR();
	}
	else if (status == IIR_RXRDY) {
		TTYkbSR();
		TTYdspSR();
	}
}

void TTYkbSR() {
	char chin;
	int wpid;
	int rpid;
	unsigned int cr3Copy;
	chin = inportb(tty.port);
	if (QueEmpty(&tty.kb_wait_que))
		return;
	EnQue(chin,&tty.echo);
	wpid = tty.kb_wait_que.que[0];	// reading, not DeQue-ing
	cr3Copy = get_cr3();
	set_cr3(pcb[wpid].Dir);
	if (chin != '\r') {
		*tty.kb_str = chin;
		tty.kb_str++;
	}
	else {
		EnQue('\n', &tty.echo);
		*tty.kb_str = '\0';
		rpid = DeQue(&tty.kb_wait_que);
		EnQue(rpid, &ready_que);
		pcb[rpid].state = READY;
	}
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
	int row;
	char *str = (char *) pcb[run_pid].tf_p->ebx;

	if (pcb[run_pid].STDOUT == TTY)				// TTY
	{
		// copy the string address to the 'str" in 'tty'
		tty.dsp_str=str;
		// suspend the process in the wait queue of 'tty'
		EnQue(run_pid, &tty.dsp_wait_que);
		pcb[run_pid].state = IO_WAIT;
		run_pid = NONE;
		TTYdspSR();
	}
	else if (pcb[run_pid].STDOUT == CONSOLE)	// CONSOLE
	{
		while (*str != '\0')
		{
			if (*str == '\r')
			{
				row = (sys_cursor - VIDEO_START)/80;
				sys_cursor = (row + 1) * 80 + VIDEO_START;
			}
			else
			{
				*sys_cursor = VGA_MASK_VAL | *str;
				//increment the cursor and string position
				sys_cursor++;
			}
			str++;
		}
		// if sys_cursor exceeds the end of the screen, clear and set to VIDEO_START
		if (sys_cursor >= VIDEO_END)
		{
			sys_cursor = VIDEO_START;
			while (sys_cursor != VIDEO_END)
			{
				*sys_cursor = VGA_MASK_VAL | ' ';
				sys_cursor++;
			}
			sys_cursor = VIDEO_START;
		}
	}
	else	// panic
	{
		KPANIC_UCOND("Panic: no such device!");
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
	pcb[pid].Dir = KDir; //Kernel initialized & forked process are run in real memory

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
	int i;
	int ppid = pcb[run_pid].ppid;
	int ec = pcb[run_pid].tf_p->ebx;

	if(pcb[ppid].state == WAIT)
	{
		//parent was waiting on us we need to wake it up and set its wait returns
		pcb[ppid].state = READY;
		EnQue(ppid, &ready_que);
		set_cr3(pcb[ppid].Dir);		//switch address space to parents address space
		*((int*)pcb[ppid].tf_p->ebx) = ec;
		pcb[ppid].tf_p->ebx = run_pid; //exiting program
		set_cr3(pcb[run_pid].Dir);	//restore old address space
		//cleanup self
		pcb[run_pid].state = AVAIL;
		EnQue(run_pid, &avail_que);
		//wipe out own pages
		for(i=0;i<PAGE_MAX;i++)
			if(pages[i].pid==run_pid) pages[i].pid = NONE;
	}
	else
	{
		//zombify self and wait for the parent
		pcb[run_pid].state = ZOMBIE;
		if (pcb[ppid].signal_handler[SIGCHLD] != 0)
			AlterStack(ppid, pcb[ppid].signal_handler[SIGCHLD]);
	}
	run_pid = NONE;
}

void SysWait(void)
{
	int i = 0;
	int zpid = NONE;
	int exCode;
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
		set_cr3(pcb[zpid].Dir);	//switch to zombie childs address space
		exCode = pcb[zpid].tf_p->ebx;
		set_cr3(pcb[run_pid].Dir);	//switch back to parent's address space
		*((int*)pcb[run_pid].tf_p->ebx) = exCode; //exit code
		pcb[run_pid].tf_p->ebx = zpid; //exiting program
		//cleanup child
		pcb[zpid].state = AVAIL;
		EnQue(zpid, &avail_que);
		//wipe out own pages
		for(i=0;i<PAGE_MAX;i++)
			if(pages[i].pid==run_pid) pages[i].pid = NONE;
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

	if(tPid == 0 && sigNo == SIGCONT)
	{
		for(i = 0; i < PROC_MAX; i++)
		{
			if(pcb[i].ppid == run_pid && pcb[i].state == SLEEP)
			{
				pcb[i].state = READY;
				EnQue(i, &ready_que);
			}
		}
	}
}

void SysRead(void)
{
	if (pcb[run_pid].STDIN == CONSOLE) {
		char nc;
		if (!QueEmpty(&kb.buffer))			// checks if kb buffer is empty
		{
			nc = (char)DeQue(&kb.buffer);		// takes next char in the buffer, BL from EBX
			pcb[run_pid].tf_p->ebx = (int) nc;	// places BL back into EBX
		}
		else
		{
			EnQue(run_pid, &kb.wait_que);
			pcb[run_pid].state = IO_WAIT;
			run_pid = NONE;
		}
	}
	else if (pcb[run_pid].STDIN == TTY) {
		//tty.kb_str =(char *) &(pcb[run_pid].tf_p->ebx);
		tty.kb_str =(char *) pcb[run_pid].tf_p->ebx;
		EnQue(run_pid, &tty.kb_wait_que);
		pcb[run_pid].state = IO_WAIT;
		run_pid = NONE;
	}
	else {
		KPANIC_UCOND("Kernel Panic: no such device");
		breakpoint();
	}
}

void SysVfork(void)
{
	int Dir=0, IT=0, DT=0, IP=0, DP=0;
	int i;
	int dFlags = RW|PRESENT;

	//process creation stuff
	int pid = DeQue(&avail_que);
	func_p_t launchPtr = (func_p_t)pcb[run_pid].tf_p->ebx;
	//queue new process
	EnQue(pid, &ready_que);
	// pcb copied and updated with new values
	MemCpy((char *)&pcb[pid],(char *)&pcb[run_pid], sizeof(pcb_t));
	pcb[pid].state = READY;
	pcb[pid].wake_time = 0;
	pcb[pid].time_count = 0;
	pcb[pid].ppid = run_pid;

	//find pages
	for(i=16;i<PAGE_MAX;i++)
	{
		if(pages[i].pid!=NONE) continue;
		else if(!Dir) Dir=i;
		else if(!IT) IT=i;
		else if(!DT) DT=i;
		else if(!IP) IP=i;
		else if(!DP) {DP=i; break;}
	}
	//if we couldn't find 5 free pages then panic
	KPANIC(!(Dir&&IT&&DT&&IP&&DP),"Panic: out of memory, can't vfork process!\n");

	//clear pages and set ownership
	pages[Dir].pid=pages[IT].pid=pages[DT].pid=pages[IP].pid=pages[DP].pid=pid;
	Bzero(pages[Dir].u.content, PAGE_SIZE);
	Bzero(pages[IT].u.content, PAGE_SIZE);
	Bzero(pages[DT].u.content, PAGE_SIZE);
	Bzero(pages[IP].u.content, PAGE_SIZE);
	Bzero(pages[DP].u.content, PAGE_SIZE);

	//constructing VM translation tables
	//Dir page
	//Copy kernel mapping to each processes VM space
	MemCpy(pages[Dir].u.content, (char*)KDir, 16*sizeof(pages[Dir].u.entry[0]));
	pages[Dir].u.entry[256] = pages[IT].u.addr|dFlags;	//copy table for pg w/ instructions
	pages[Dir].u.entry[511] = pages[DT].u.addr|dFlags;	//copy table for pg w/ stack
	pcb[pid].Dir = pages[Dir].u.addr;

	//IT page
	pages[IT].u.entry[0] = pages[IP].u.addr|dFlags;		//link ins table

	//DT page
	pages[DT].u.entry[1023] = pages[DP].u.addr|dFlags;	//link data table

	//IP page
	MemCpy(pages[IP].u.content, (char*)launchPtr, PAGE_SIZE);		//copying code to Ins Page

	//DP page
	pcb[pid].tf_p = (tf_t*)((char*)G2-sizeof(*pcb[pid].tf_p));
	pages[DP].u.entry[1023] = EF_DEFAULT_VALUE|EF_INTR;
	pages[DP].u.entry[1022] = get_cs();
	pages[DP].u.entry[1021] = G1;
}
