// kernel.c, 159, phase 1
//
// Team Name: PetrinOS (Members: 2)

#include "spede.h"
#include "const-type.h"
#include "entry.h"		// entries to kernel (TimerEntry, etc.)
#include "tools.h"		// small handy functions
#include "ksr.h"		// kernel service routines
#include "proc.h"		// all user process code here

// declare kernel data
int run_pid;					// current running PID
unsigned short *sys_cursor;		// pointer to the current place pointed to by the cursor
que_t avail_que, ready_que;		// avail PID and those created/ready to run
pcb_t pcb[PROC_MAX];			// Process Control Blocks
unsigned int sys_time_count;	// total time system has been up
unsigned int sys_rand_count;
struct i386_gate *idt;			// interrupt descriptor table
mutex_t video_mutex;
kb_t kb;
unsigned int KDir;
page_t page[PAGE_MAX]

void BootStrap(void)	// set up kernel!
{
	int i;
	sys_time_count=0;
	sys_cursor = VIDEO_START;
	sys_rand_count = 0;

	Bzero((char *)&avail_que, sizeof(que_t));
	Bzero((char *)&ready_que, sizeof(que_t));
	Bzero((char *)&video_mutex, sizeof(mutex_t));
	Bzero((char *)&kb, sizeof(kb_t));
	//enqueue all the available PID numbers to avail queue(none yet)
	for(i=0;i<PROC_MAX;i++) EnQue(i,&avail_que);

	//setup IDT/get IDT location
	idt = get_idt_base();
	//setup IDT events
	fill_gate(&idt[TIMER_EVENT], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&idt[SYSCALL_EVENT], (int)SyscallEntry, get_cs(), ACC_INTR_GATE, 0);

	//virtual memory initialization
	KDir = get_cr3();
	for(i=0;i<PAGE_MAX;i++)
	{
		page[i].u.addr = DRAM_START+(i*PAGE_SIZE);
	}

	//send PIC control register the mask value for timer handling
	outportb(PIC_MASK_REG, PIC_MASK_VAL);
}

int main(void)		// OS starts
{
	//do the boot strap things 1st
	BootStrap();

	SpawnSR(Idle);	// create Idle thread
	SpawnSR(Login);
	run_pid = IDLE;
	Loader(pcb[run_pid].tf_p);

	return 0;		// never would actually reach here
}

void Scheduler(void)	// choose a run_pid to run
{
	if(run_pid > IDLE) return;	// a user PID is already picked

	if(QueEmpty(&ready_que))
	{
		run_pid = IDLE;		// use the Idle thread
	}
	else
	{
		pcb[IDLE].state = READY;
		run_pid = DeQue(&ready_que);	// pick a different proc
	}

	pcb[run_pid].time_count = 0;	// reset runtime count
	pcb[run_pid].state = RUN;
}

void Kernel(tf_t *tf_p)		// kernel runs
{
	//copy tf_p to the trapframe ptr (in PCB) of the process in run
	pcb[run_pid].tf_p = tf_p;

	//handle the interrupt by invoking hte appropriate event
	switch(pcb[run_pid].tf_p->event)
	{
	case TIMER_EVENT:
		TimerSR();
		break;
	case SYSCALL_EVENT:
		SyscallSR();
		break;
	default:
		KPANIC_UCOND("Kernel Panic: no such event!\n");
	}

	KBSR();	//if(cons_kbhit()&&cons_getchar()=='b') breakpoint();

	Scheduler();
	Loader(pcb[run_pid].tf_p);
}

