/* $Id$ */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fbuff.h"

fbuff
InitFBuff(int size)
{
	fbuff lb;

	lb = (fbuff)malloc(FBUFF_SIZE);

	if(lb == NULL)
	{
		fprintf(stderr,"InitFBuff: couldn't get space for fbuff\n");
		fflush(stderr);
		return(NULL);
	}

	/*
	 * count from bottom to top
	 *
	 * head is the next available space
	 * tail is the last valid data item
	 */
	lb->size = size;
	lb->head = size - 1;
	lb->tail = 0;

	lb->vals = (double *)malloc(size*sizeof(double));
	if(lb->vals == NULL)
	{
		fprintf(stderr,"InitFBuff: couldn't get space for vals\n");
		fflush(stderr);
		free(lb);
		return(NULL);
	}

	return(lb);

}

void
FreeFBuff(fbuff fb)
{
	free(fb->vals);
	free(fb);
	
	return;
}

void
UpdateFBuff(fbuff fb, double val)
{
	F_HEAD(fb) = val;
	fb->head = MODMINUS(fb->head,1,fb->size);
	
	/*
	 * if we have moved the head over the tail, bump the tail around too
	 */
	if(fb->head == fb->tail)
	{
		fb->tail = MODMINUS(fb->tail,1,fb->size);
	}
	
	return;
}

