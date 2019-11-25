// kernel.c, 159, phase 1
//
// Team Name: PetrinOS (Members: 2)

#include "spede.h"
#include "const-type.h"
#include "entry.h"		// entries to kernel (TimerEntry, etc.)
#include "tools.h"		// small handy functions
#include "ksr.h"		// kernel service routines
#include "proc.h"		// all user process code here

void TTYinit(void);

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
page_t pages[PAGE_MAX];
tty_t tty;

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
	Bzero((char *)&tty, sizeof(tty_t));
	//enqueue all the available PID numbers to avail queue(none yet)
	for(i=0;i<PROC_MAX;i++) EnQue(i,&avail_que);

	//setup IDT/get IDT location
	idt = get_idt_base();
	//setup IDT events
	fill_gate(&idt[TIMER_EVENT], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&idt[SYSCALL_EVENT], (int)SyscallEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&idt[TTY_EVENT], (int)TTYEntry, get_cs(), ACC_INTR_GATE, 0);

	//virtual memory initialization
	KDir = get_cr3();
	for(i=0;i<PAGE_MAX;i++)
	{
		pages[i].pid = NONE;
		pages[i].u.addr = DRAM_START+(i*PAGE_SIZE);
	}

	//send PIC control register the mask value for timer handling
	outportb(PIC_MASK_REG, PIC_MASK_VAL);
}

int main(void)		// OS starts
{
	//do the boot strap things 1st
	BootStrap();
	TTYinit();	// init. term. port

	SpawnSR(Idle);	// create Idle thread
	SpawnSR(Login);
	run_pid = IDLE;
	set_cr3(pcb[run_pid].Dir);
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
	case TTY_EVENT:
		TTYSR();
		break;
	default:
		KPANIC_UCOND("Kernel Panic: no such event!\n");
	}

	KBSR();	//if(cons_kbhit()&&cons_getchar()=='b') breakpoint();

	Scheduler();
	set_cr3(pcb[run_pid].Dir);
	Loader(pcb[run_pid].tf_p);
}

void TTYinit(void) {
	int i, j;

	Bzero((char *)&tty, sizeof(tty_t));
	tty.port = TTY0;

	outportb(tty.port+CFCR, CFCR_DLAB);             // CFCR_DLAB is 0x80
	outportb(tty.port+BAUDLO, LOBYTE(115200/9600)); // period of each of 9600 bauds
	outportb(tty.port+BAUDHI, HIBYTE(115200/9600));
	outportb(tty.port+CFCR, CFCR_PEVEN|CFCR_PENAB|CFCR_7BITS);

	outportb(tty.port+IER, 0);
	outportb(tty.port+MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);

	for(i=0; i<166667; i++)asm("inb $0x80");       // wait .1 sec
	outportb(tty.port+IER, IER_ERXRDY|IER_ETXRDY); // enable TX & RX intr
	for(i=0; i<166667; i++)asm("inb $0x80");       // wait .1 sec

	for(j=0; j<3; j++) {                           // clear 3 lines
		outportb(tty.port, 'V');
		for(i=0; i<83333; i++)asm("inb $0x80");     // wait .5 sec should do
		outportb(tty.port, '\n');
		for(i=0; i<83333; i++)asm("inb $0x80");
		outportb(tty.port, '\r');
		for(i=0; i<83333; i++)asm("inb $0x80");
	}
	inportb(tty.port);                             // get 1st key PROCOMM logo
	for(i=0; i<83333; i++)asm("inb $0x80");        // wait .5 sec
}
