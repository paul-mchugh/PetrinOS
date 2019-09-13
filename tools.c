// tools.c, 159

//this .c code needs to include spede.h, const-type.h, and ext-data.h
#include "tools.h"
#include "spede.h"
#include "const-type.h"
#include "ext-data.h"

//code a QueEmpty() function that checks whether a queue (located by a
//given pointer) is emptyr; returns 1 if yes, or 0 if not
int QueEmpty(que_t* que)
{
	return que->tail==0;
}

//similarly, code a QueFull() function to check for being full or not
int QueFull(que_t* que)
{
	return que->tail==QUE_MAX;
}

//code a DeQue() function that dequeues the 1st number in the queue (given
//by a pointer); if empty, return constant NONE (-1)
int DeQue(que_t* que)
{
	int bak;
	int i;

	if(que->tail==-1) return -1;
	bak = que->que[que->tail];
	que->tail--;
	MemCpy((char*)&que->que[0], (char*)&que->que[1], (QUE_MAX-1)*sizeof(int));
	return bak;
}


//code an EnQue() function given a number and a queue (by a pointer), it
//appends the number to the tail of the queue, or shows an error message
//and go into the GDB:
void EnQue(int data, que_t* que)
{
	KPANIC(que->tail==MAX_QUE, "Panic: queue is full, cannot EnQue!\n");

	que->que[que->tail] = data;
	que->tail++;
}

//code a Bzero() function to clear a memory region (by filling with NUL
//characters), the beginning of the memory location will be given via a
//character pointer, and the size of the memory will be given by an unsigned
//int 'max'
void Bzero(char* ptr, unsigned int max)
{
	char* end;

	end = ptr + max;
	while(ptr!=end) *ptr=0;
}

//code a MemCpy() function to copy a memory region located at a given
//character pointer 'dst,' from the starting location at a given character
//pointer 'src,' the size will also be given as an unsigned integer 'max'
void MemCpy(char* dst, char* src, unsigned int max)
{
	int i;
	//if this function should have no effect return now
	//do not remove the following check doing so will cause a bug when
	//max==0 and src>=0. B/C in the backwards copy case i will underflow
	//to MAX_INT if max==0
	if(src==dst || max==0) return;

	//this determines the direction in which we copy the data.
	//doing so allows us to copy to overlapping arrays
	if(dst>src)
	{
		//if dst's address is greater than that of src then we copy
		//from start to end
		for(i=0;i<max;i++) dst[i]=src[i];
	}
	else
	{
		//if dst's address is less than that of src then we copy from
		//end to start
		for(i=max-1;i>=0;i--) dst[i]=src[i];
	}
}
