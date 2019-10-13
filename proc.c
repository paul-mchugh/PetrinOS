// proc.c, 159
// processes are coded here

#include "const-type.h"
#include "ext-data.h"
#include "proc.h"
#include "syscall.h"
#include "tools.h"

/*
Code an Idle() function that doesn't have any input or return, but just
flickers the dot symbol at the upper-left corner of the target PC display.

It reads sys_time_count and at each second interval writes '*' or ' ' via
an unsigned short pointer to the VGA video memory location 0xb8000.
Apply the VGA_MASK_VAL when showing * or space so it will appear to be
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
	unsigned short* vga = VIDEO_START;
	while(1)
	{
		sys_rand_count++;
		if(sys_time_count % 100 == 0)
		{
			if(flag == 0)
			{
				*vga = VGA_MASK_VAL | '*';
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

void Init(void)
{
	char pid_str[20], str[20];
	int my_pid, forked_pid;
	int i, col, exit_pid, exit_code;
	unsigned int sleep_period, total_sleep_period;

	for(i=0;i<5;i++)
	{
		forked_pid = sys_fork();
		if(forked_pid==0)break;
		if(forked_pid==NONE)
		{
			sys_write("sys_fork() failed!\n");
		}
	}

	my_pid = sys_get_pid();
	Number2Str(my_pid, pid_str);

	if(forked_pid!=0)
	{
		for(i=0;i<5;i++)
		{
			exit_pid = sys_wait(&exit_code);

			sys_lock_mutex(VIDEO_MUTEX);
			sys_set_cursor(my_pid,i*14);
			sys_write("PID ");
			Number2Str(exit_pid,str);
			sys_write(str);
			sys_write(": ");
			Number2Str(exit_code,str);
			sys_write(str);
			sys_unlock_mutex(VIDEO_MUTEX);
		}
		sys_write("  Init exits.");
		sys_exit(0);
	}
	else
	{
		total_sleep_period = 0;
		for (col = 0; col < 70; col++)
		{
			sys_lock_mutex(VIDEO_MUTEX);
			sys_set_cursor(my_pid, col);
			sys_write(pid_str);
			sys_unlock_mutex(VIDEO_MUTEX);
			sleep_period = (sys_get_rand()/my_pid % 4) + 1;
			sys_sleep(sleep_period);
			total_sleep_period += sleep_period;
		}
		sys_exit(total_sleep_period);
	}
}
