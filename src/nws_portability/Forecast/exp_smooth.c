/* $Id$ */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>

#include "fbuff.h"
#include "exp_smooth.h"

struct exp_smooth_state
{
	fbuff series; 			/* the series so far */
	fbuff time_stamps;		/* the time stamps */
	double last_pred;
	double gain;
};

/*
 * init local state.  can save a copy of the pointer to the
 * series and time stamps, if desired
 */
char *
InitExpSmooth(fbuff series, fbuff time_stamps, char *params)
{
	struct exp_smooth_state *state;
	double gain;
	char *p_str;
	
	state = (struct exp_smooth_state *)
		malloc(sizeof(struct exp_smooth_state));
	
	if(state == NULL)
	{
		return(NULL);
	}
	
	if(params == NULL)
	{
		free(state);
		return(NULL);
	}
	
	p_str = params;
	gain = strtod(p_str,&p_str);
	
	/*
	 * all functions take a series and time_stamps
	 */
	state->series = series;
	state->time_stamps = time_stamps;
	state->last_pred = 0.0;
	state->gain = gain;
	
	return((char *)state);
}

void
FreeExpSmooth(char *state)
{
	free(state);
	return;
}

void
UpdateExpSmooth(char *state,
		double ts,
		double value)
{
	double pred;
	struct exp_smooth_state *s = (struct exp_smooth_state *)state;

	pred = s->last_pred + s->gain * (value - s->last_pred);
	
	/*
	 * if there is only one value, last pred is the first valeu in the
	 * series
	 */
	if(F_COUNT(s->series) <= 1)
	{
		s->last_pred = F_VAL(s->series,F_FIRST(s->series));
	}
	else
	{
		s->last_pred = pred;
	}

	return;
}

int
ForcExpSmooth(char *state, double *v)
{
	struct exp_smooth_state *s = (struct exp_smooth_state *)state;
	
	*v = s->last_pred;

	return(1);

}


