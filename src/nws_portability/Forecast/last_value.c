/* $Id$ */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>

#include "fbuff.h"
#include "last_value.h"

struct last_value_state
{
	fbuff series; 			/* the series so far */
	fbuff time_stamps;		/* the time stamps */
	double value;
};

/*
 * init local state.  can save a copy of the pointer to the
 * series and time stamps, if desired
 */
char *
InitLastValue(fbuff series, fbuff time_stamps, char *params)
{
	struct last_value_state *state;
	
	state = (struct last_value_state *)
		malloc(sizeof(struct last_value_state));
	
	if(state == NULL)
	{
		return(NULL);
	}
	
	/*
	 * all functions take a forcb
	 */
	state->series = series;
	state->time_stamps = time_stamps;
	
	return((char *)state);
}

void
FreeLastValue(char *state)
{
	free(state);
	return;
}

void
UpdateLastValue(char *state,
		double ts,
		double value)
{
	/*
	 * no op -- will just grab it from series
	 */
	return;
}

int
ForcLastValue(char *state, double *v)
{
	double val;
	struct last_value_state *s = (struct last_value_state *)state;
	
	if(!IS_EMPTY(s->series))
	{
		val = F_VAL(s->series,F_FIRST(s->series));
		*v = val;
		return(1);
	}
	else
	{
		*v = 0.0;
		return(0);
	}
}


