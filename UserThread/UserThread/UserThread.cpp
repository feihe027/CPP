// UserThread.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include < assert.h > 
#include < stdlib.h > 
#include <setjmp.h> 
#include < stdio.h > 
#include < Windows.h > 

typedef void (* PThreadFun)(void *);

typedef struct _SCHED_SLOT
{
	jmp_buf buf;
	int has_set;
	PThreadFun thread;
	void * arg;
	char * stack;
} SCHED_SLOT;


typedef struct _SLOT_LIST
{
	SCHED_SLOT slot;
	struct _SLOT_LIST*  next;
}SLOT_LIST;

typedef struct _SCHED_SYSTEM
{
	SLOT_LIST * threads;
	SLOT_LIST * current;
	int main_esp;
	int main_ebp;
	char * unfreed_stack;
	int retaddr;
} SCHED_SYSTEM;


#define CHANGE_STACK(newaddr) _asm mov esp, newaddr
#define STACK_SIZE 65536 
#define RESERVED_STACK 4 


SCHED_SYSTEM g_sys;

void RegThreadFun(PThreadFun fun, void * arg)
{
	SLOT_LIST * new_thread = (SLOT_LIST *)malloc(sizeof(SLOT_LIST));
	new_thread->next = g_sys.threads;
	g_sys.threads = new_thread;
	new_thread->slot.arg = arg;
	new_thread->slot.has_set = 0;
	new_thread->slot.stack = 0;
	new_thread->slot.thread = fun;
}

void FreeStack()
{
	if (g_sys.unfreed_stack)
	{
		free(g_sys.unfreed_stack);
		g_sys.unfreed_stack = 0;
	}
}
void start_thread(SLOT_LIST * iter);


void schedule()
{
	SLOT_LIST * old;
	FreeStack();
	old = g_sys.current;
	g_sys.current = g_sys.current->next;
	if (!g_sys.current)
	{
		g_sys.current = g_sys.threads;
	}

	if (!setjmp(old->slot.buf))
	{
		old->slot.has_set = 1;

		if (g_sys.current->slot.has_set)
			longjmp(g_sys.current->slot.buf, 1);
		else 
			start_thread(g_sys.current);
	}
}


static void exit_thread()
{
	SLOT_LIST * iter;
	FreeStack();
	if (g_sys.current == g_sys.threads)
	{
		g_sys.threads = g_sys.threads->next;
		g_sys.unfreed_stack = g_sys.current->slot.stack;
		free(g_sys.current);
		g_sys.current = g_sys.threads;
	}
	else 
	{
		for (iter = g_sys.threads; iter && iter->next != g_sys.current && iter->next !=  0; iter = iter->next)
			;
		assert (iter && iter->next  == g_sys.current);
		iter->next = g_sys.current->next;
		g_sys.unfreed_stack = g_sys.current->slot.stack;
		free(g_sys.current);
		g_sys.current = iter->next;
	}

	if (g_sys.current ==  0)
	{
		g_sys.current = g_sys.threads;
	}

	if (g_sys.current)
	{
		if (g_sys.current->slot.has_set)
			longjmp(g_sys.current->slot.buf, 1);
		else 
			start_thread(g_sys.current);
	}
}

static jmp_buf g_jmpBuff;

static void start_thread(SLOT_LIST * iter)
{
	char * stack_btm;
	static PThreadFun thread;
	static void * arg;

	iter->slot.stack = (char *)malloc(STACK_SIZE + RESERVED_STACK);
	stack_btm = iter->slot.stack + STACK_SIZE;
	thread = iter->slot.thread;
	arg = iter->slot.arg;
	CHANGE_STACK(stack_btm);
	thread(arg);
	if (g_sys.threads->next)
		exit_thread();
	else 
	{
		g_sys.unfreed_stack = g_sys.threads->slot.stack;
		free(g_sys.threads);
		longjmp(g_jmpBuff, 1);
	}
}

void start_first_thread()
{
	if (!setjmp(g_jmpBuff))
	{
		g_sys.current = g_sys.threads;
		start_thread(g_sys.current);
	}
	FreeStack();
}


void  Fun0(void * p)
{
	for (int i = 0; i <  3; ++ i)
	{
		printf(" %d, %d\n " , 0 , i); 
		Sleep(200);
		schedule();
	}
}

void  Fun1(void * p)
{
	for (int i = 0; i <  6; ++ i)
	{
		printf(" %d, %d\n " , 1 , i);
		Sleep(200);
		schedule();
	}
}

void Fun3(void * p)
{
	for (int i = 0; i <  6; ++ i)
	{
		printf(" %d, %d\n " , 3 , i); 
		Sleep(200);
		schedule();
	}
}


void  Fun2(void * p)
{
	for (int i = 0;i <  12; ++ i)
	{
		printf(" %d, %d\n " , 2 , i); 
		Sleep(200);
		schedule();
		if (i ==  8)
		{
			RegThreadFun(Fun3, 0);
		}
	}
}


int main()
{
	RegThreadFun(Fun0, 0);
	RegThreadFun(Fun1, 0);
	RegThreadFun(Fun2, 0);

	start_first_thread();
	printf(" finished\n ");
	getchar();
}

