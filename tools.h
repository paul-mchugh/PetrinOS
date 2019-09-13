// tools.h, 159

#ifndef _TOOLS_
#define _TOOLS_

#include "const-type.h" // need definition of 'que_t' below

#define KPANIC_UCOND(S) cons_printf("%s",S); breakpoint()
#define KPANIC(C,S) if(C) { KPANIC_UCOND(S); }

int		QueEmpty(que_t* que)
int		QueFull(que_t* que)
int		DeQue(que_t *que);
void	EnQue(int data, que_t *que);
void	Bzero(char* ptr, unsigned int max)
void	MemCpy(char* dst, char* src, unsigned int max)

#endif

