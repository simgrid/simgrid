/* $Id$ */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>

#include "fbuff.h"
#include "run_mean.h"

struct run_mean_state
{
	fbuff series; 			/* the series so far */
	fbuff time_stamps;		/* the time stamps */
	double total;
	double count;
};

/*
 * init local state.  can save a copy of the pointer to the
 * series and time stamps, if desired
 */
char *
InitRunMean(fbuff series, fbuff time_stamps, char *params)
{
	struct run_mean_state *state;
	
	state = (struct run_mean_state *)
		malloc(sizeof(struct run_mean_state));
	
	if(state == NULL)
	{
		return(NULL);
	}
	
	/*
	 * all functions take a forcb
	 */
	state->series = series;
	state->time_stamps = time_stamps;
	state->total = 0.0;
	state->count = 0.0;
	
	return((char *)state);
}

void
FreeRunMean(char *state)
{
	free(state);
	return;
}

void
UpdateRunMean(char *state,
		double ts,
		double value)
{
	struct run_mean_state *s;
	
	s = (struct run_mean_state *)state;
	
	s->total += value;
	s->count += 1.0;
	return;
}

int
ForcRunMean(char *state, double *v)
{
	double val;
	struct run_mean_state *s = (struct run_mean_state *)state;
	
	if(s->count != 0.0)
	{
		val = s->total / s->count;
		*v = val;
		return(1);
	}
	else
	{
		*v = 0.0;
		return(0);
	}
}


