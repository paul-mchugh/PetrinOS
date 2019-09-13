// kernel.c, 159, phase 1
//
// Team Name: ??????? (Members: ??????...)

#include "spede.h"
#include "const-type.h"
#include "entry.h"    // entries to kernel (TimerEntry, etc.)
#include "tools.h"    // small handy functions
#include "ksr.h"      // kernel service routines
#include "proc.h"     // all user process code here

// declare kernel data
declare an integer: run_pid;  // current running PID
declare 2 queues: avail_que and ready_que;  // avail PID and those created/ready to run
declare an array of PCB type: pcb[PROC_MAX];  // Process Control Blocks

declare an unsigned integer: sys_time_count
struct i386_gate *idt;         // interrupt descriptor table

void BootStrap(void) {         // set up kernel!
   set sys time count to zero

   call tool Bzero((char *)&avail_que, sizof(que_t)) to clear avail queue
   call tool Bzero() to clear ready queue
   enqueue all the available PID numbers to avail queue

   get IDT location
   addr of TimerEntry is placed into proper IDT entry
   send PIC control register the mask value for timer handling
}

int main(void) {               // OS starts
   do the boot strap things 1st

   SpawnSR(Idle);              // create Idle thread
   set run_pid to IDLE
   call Loader() to load the trapframe of Idle

   return 0; // never would actually reach here
}

void Scheduler(void) {              // choose a run_pid to run
   if(run_pid > IDLE) return;       // a user PID is already picked

   if(QueEmpty(&ready_que)) {
      run_pid = IDLE;               // use the Idle thread
   } else {
      pcb[IDLE].state = READY;
      run_pid = DeQue(&ready_que);  // pick a different proc
   }

   pcb[run_pid].time_count = 0;     // reset runtime count
   pcb[run_pid].state = RUN;
}

void Kernel(tf_t *tf_p) {       // kernel runs
   copy tf_p to the trapframe ptr (in PCB) of the process in run

   call the timer service routine

   if 'b' key on target PC is pressed, goto the GDB prompt

   call Scheduler() to change run_pid if needed
   call Loader() to load the trapframe of the selected process
}

