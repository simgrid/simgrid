
/* log_usage - A test of normal usage of the log facilities                     */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.     */

#pragma hdrstop

#include "xbt/context.h"
#include "xbt/fifo.h"
#include "stdlib.h"
#include "stdio.h"

xbt_context_t cA = NULL;
xbt_context_t cB = NULL;
xbt_context_t cC = NULL;
xbt_fifo_t fifo = NULL;

void print_args(int argc, char** argv);
void print_args(int argc, char** argv)
{
	int i ; 
	
	printf("args=<");
	
	for(i=0; i<argc; i++) 
		printf("%s ",argv[i]);
	
	printf(">\n");
}

int fA(int argc, char** argv);
int fA(int argc, char** argv)
{
	printf("Here is fA: ");
	print_args(argc,argv);
	
	printf("\tContext A: Yield\n");
	xbt_context_yield();
	
	xbt_fifo_push(fifo,cB);
	printf("\tPush context B from context A\n");
	
	printf("\tContext A: Yield\n");
	xbt_context_yield();
	printf("\tContext A : bye\n");
	
	return 0;
}

int fB(int argc, char** argv);
int fB(int argc, char** argv)
{
	printf("Here is fB: ");
	print_args(argc,argv);
	xbt_fifo_push(fifo,cA);
	printf("\tPush context A from context B\n");
	printf("\tContext B: Yield\n");
	xbt_context_yield();
	printf("\tContext B : bye\n");
	return 0;
}

int fC(int argc, char** argv);
int fC(int argc, char** argv)
{
	printf("Here is fC: ");
	print_args(argc,argv);
	
	
	printf("\tContext C: Yield (and exit)\n");
	xbt_context_yield();
	
	
	return 0;
}
#pragma argsused

int main(int argc, char **argv)
{
	xbt_context_t context = NULL;
	
	printf("XXX Test the simgrid context API\n");
	printf("    If it fails, try another context backend.\n    For example, to force the pthread backend, use:\n       ./configure --with-context=pthread\n\n");
	
	xbt_context_init();
	
	cA = xbt_context_new(fA, NULL, NULL, NULL, NULL, 0, NULL);
	cB = xbt_context_new(fB, NULL, NULL, NULL, NULL, 0, NULL);
	cC = xbt_context_new(fC, NULL, NULL, NULL, NULL, 0, NULL);
	
	fifo = xbt_fifo_new();
	
	printf("Here is context 'main'\n");
	xbt_context_start(cA);
	printf("\tPush context 'A' from context 'main'\n");xbt_fifo_push(fifo,cA);
	xbt_context_start(cB);
	printf("\tPush context 'B' from context 'main'\n");xbt_fifo_push(fifo,cB);
	xbt_context_start(cC);xbt_fifo_push(fifo,cC);
	printf("\tPush context 'C' from context 'main'\n");xbt_fifo_push(fifo,cC);
	
	while((context=xbt_fifo_shift(fifo))) 
	{
		printf("Context main: Yield\n");
		xbt_context_schedule(context);
	}
	
	xbt_fifo_free(fifo);
	xbt_context_exit();
	
	cA=cB=cC=NULL;
	return 0;
}

