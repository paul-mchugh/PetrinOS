// proc.c, 159
// processes are coded here

#include "const-type.h"
#include "ext-data.h"
#include "proc.h"
#include "syscall.h"

/*
Code an Idle() function that doesn't have any input or return, but just
flickers the dot symbol at the upper-left corner of the target PC display.

It reads sys_time_count and at each second interval writes '.' or ' ' via
an unsigned short pointer to the VGA video memory location 0xb8000.
Apply the VGA_MASK_VAL when showing . or space so it will appear to be
boldface writing.
	... declare a pointer and set it to the upper-left display corner ...
	... declare and set a flag ...
	in an infinite loop:
		whenever the system time reaches a multiple of 100 (per 1 second):
			a. judging from the flag and show either the dot or space
			b. alternate the flag
*/

void Idle(void)
{
	int flag = 0;
	unsigned short* vga = (unsigned short*)((char*)VGA_MEM_START)+1;
	while(1)
	{
		if(sys_time_count % 100 == 0)
		{
			if(flag == 0)
			{
				*vga = VGA_MASK_VAL | '.';
				flag = 1;
			}
			else
			{
				*vga = VGA_MASK_VAL | ' ';
				flag = 0;
			}
		}
	}
}

void Init(void) {
	int my_pid, os_time;
	char pid_str[20];
	char time_str[20];
	
	cons_printf("hi!");
	my_pid = sys_get_pid();
	Number2Str(my_pid, pid_str);
	while (1) {
		sys_write("my PID is ");
		sys_write(pid_str);
		sys_write("... ");
		sys_sleep(1);
		os_time = sys_get_time();
		Number2Str(os_time, time_str);
		sys_write("sys time is ");
		sys_write(time_str);
		sys_write("... ");
		sys_sleep(1);
	}
}
