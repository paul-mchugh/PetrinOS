// proc.c, 159 processes are coded here
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
	int i, pid, forked, col, total_sleep_period, sleep_period;
	char pid_str[20];

	sys_signal(SIGCHLD, MyChildExitHandler);
	for(i = 0; i < 5; i++)
	{
		forked = sys_fork();
		if (forked == 0)
			break;	// children don't loop!
		if (forked == NONE)
		{
			sys_write("sys_fork() failed\n");
			sys_exit(NONE);
		}
	}
	pid = sys_get_pid();
	Number2Str(pid, pid_str);
	if (forked > 0)
	{
		sys_sleep(10);
		sys_kill(0, SIGCONT);
		while(1)
		{
			sys_lock_mutex(VIDEO_MUTEX);
			sys_set_cursor(pid, 0);
			sys_write(pid_str);
			sys_unlock_mutex(VIDEO_MUTEX);

			sys_sleep(10);

			sys_lock_mutex(VIDEO_MUTEX);
			sys_set_cursor(pid, 0);
			sys_write("-");
			sys_unlock_mutex(VIDEO_MUTEX);

			sys_sleep(10);
		}
	}
	sys_sleep(1000000);
	total_sleep_period = 0;
	col = 0;
	for (col = 0; col < 70; col++)
	{
		sys_lock_mutex(VIDEO_MUTEX);
		sys_set_cursor(pid, col);
		sys_write(pid_str);
		sys_unlock_mutex(VIDEO_MUTEX);

		sleep_period = (sys_get_rand()/pid % 4) + 1;
		sys_sleep(sleep_period);
		total_sleep_period += sleep_period;

		sys_lock_mutex(VIDEO_MUTEX);
		sys_set_cursor(pid, col);
		sys_write(".");
		sys_unlock_mutex(VIDEO_MUTEX);
	}
	sys_exit(total_sleep_period);
}

// putting it here because that makes sense
void MyChildExitHandler(void)
{
	char ecStr[20], childPidStr[20];
	int pid, childPid, ec;

	childPid = sys_wait(&ec);
	pid = sys_get_pid();
	Number2Str(childPid, childPidStr);
	Number2Str(ec, ecStr);
	sys_lock_mutex(VIDEO_MUTEX);
	sys_set_cursor(childPid, 72);
	sys_write(childPidStr);
	sys_write(":");
	sys_write(ecStr);
	sys_unlock_mutex(VIDEO_MUTEX);
}

void Login(void)
{
	char login_str[STR_MAX], passwd_str[STR_MAX];
	while (1)
	{
		sys_write("login: ");
		sys_read(login_str);
		sys_write("passwd: ");
		sys_read(passwd_str);
		if (StrCmp(login_str, passwd_str)==0)		// security >_<
		{
			sys_write("login successful!\r");
			break;	// phase8
		}
		else
		{
			sys_write("login failed!\r");
		}
	}
	sys_vfork(Shell);		// phase8
	sys_exit(0);
}

void Shell(void)
{
	char command[STR_MAX], ePidStr[STR_MAX], progEcStr[STR_MAX];
	func_p_t prog = NULL;
	int progEc;
	int ePid;
	for(;;)
	{
		sys_write("PetrinOS> ");
		sys_read(command);

		//determine program to run/whether to print help
		if(StrCmp(command,"dir")==0)prog=ShellDir;
		else if(StrCmp(command,"cal")==0)prog=ShellCal;
		else if(StrCmp(command,"roll")==0)prog=ShellRoll;
		else prog=NULL;

		//start program if not NULL/print error if NULL
		if(prog)
		{
			sys_vfork(prog);
			ePid = sys_wait(&progEc);
			Number2Str(ePid, ePidStr);
			Number2Str(progEc, progEcStr);
			sys_write("   Exited Pid: ");
			sys_write(ePidStr);
			sys_write("   Exit Code: ");
			sys_write(progEcStr);
			sys_write("\r");
		}
		else
		{
			sys_write("   Valid commands are:\r      dir -- displays directory content.\r      cal -- displays calendar.\r      roll -- roll a pair of dice.\r");
		}
	}
}

void ShellDir(void)
{
	sys_write("\
   drwx 4096 ./\r\
   drwx 4096 ../\r\
   drwx   28 README.md/\r\
   drwx 1311 GDB159.RC/\r\
   drwx   40 .gitignore/\r\
");
	sys_exit(0);
}

void ShellCal(void)
{
	sys_write("\
      November 2019    \r\
                   1  2\r\
    3  4  5  6  7  8  9\r\
   10 11 12 13 14 15 16\r\
   17 18 19 20 21 22 23\r\
   24 25 26 27 28 29 30\r\
");
	sys_exit(0);
}
void ShellRoll(void)
{
	int r1, r2;
	r1 = sys_get_rand()%6+1;
	sys_sleep(1);//sleeping to give Idle a chance to run and change the random #
	r2 = sys_get_rand()%6+1;
	sys_exit(r1+r2);
}
