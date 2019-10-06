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
	int col, my_pid, forked_pid, rand;
	char pid_str[20];
	char blank = ' ';

	forked_pid = sys_fork();
	if(forked_pid == NONE) sys_write("sys_fork() failed!\n");
	forked_pid = sys_fork();
	if(forked_pid == NONE) sys_write("sys_fork() failed!\n");

	my_pid = sys_get_pid();
	Number2Str(my_pid, pid_str);
	while (1)
	{
		for (col = 0; col < 70; col++)
		{
			sys_lock_mutex(VIDEO_MUTEX);
			sys_set_cursor(my_pid, col);
			sys_write(pid_str);
			sys_unlock_mutex(VIDEO_MUTEX);
			rand = (sys_get_rand() % 4) + 1;
			sys_sleep(rand);
		}
		sys_lock_mutex(VIDEO_MUTEX);
		for (col = 0; col < 70; col++)
		{
			sys_set_cursor(my_pid, col);
			sys_write(&blank);
		}
		sys_unlock_mutex(VIDEO_MUTEX);
		sys_sleep(30);
	}
}
