/* $Id$ */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <math.h>
#include <stdio.h>
#include <string.h>


#include "fbuff.h"
#include "median.h"


/*
 * to do --
 * 
 * init M_ts to BIG_TS
 * 
 * handle the case where data is filling the window
 * 
 * find the biggest window
 */

#define SMALL_TS (-1)
#define BIG_TS (99999999999999999.99)



/*
 * state record for median and trimmed median
 */
struct median_state
{
	fbuff series;
	fbuff time_stamps;
	double M_array[MAX_MED_SIZE];
	double M_ts[MAX_MED_SIZE];
	int artificial_time;
	int M_count;
	int M_size;
	double trim;
};

/*
 * state record for adjusted median and adjusted mean
 */
struct ad_median_state
{
	fbuff series;
	fbuff time_stamps;
	double M_array[MAX_MED_SIZE];
	double M_ts[MAX_MED_SIZE];
	int M_count;
	int artificial_time;
	int last_win;
	int win;
	int min;
	int max;
	int offset;
};

void
MSort(double *M_value, double *M_ts, double value, double ts, int size)
{
	int i;
	int prev;
	int curr;
	int next;
	int l_index = 0;
	double temp;
	double min_ts = BIG_TS;
	
	/*
	 * list is sorted from smallest to biggest -- find the
	 * oldest time stamp
	 */
	for(i=0; i < size; i++)
	{
		if(M_ts[i] < min_ts)
		{
			l_index = i;
			min_ts = M_ts[i];
		}
	}
	
	/*
	 * overwrite the slot with the new value and
	 * timestamp
	 */
	M_value[l_index] = value;
	M_ts[l_index] = ts;
       
	/*
	 * bump the value into the right place in the list
	 */

	prev = l_index - 1;
	if(prev < 0)
		prev = 0;
	curr = l_index;
	next = l_index + 1;
	if(next > (size - 1))
		next = size - 1;
	/*
	 * while the value is not in the right spot
	 */
	while(!((M_value[curr] >= M_value[prev]) &&
		(M_value[curr] <= M_value[next])))
	{
		/*
		 * if it is too big, shift up
		 */
		if(M_value[curr] < M_value[prev])
		{
			temp = M_value[prev];
			M_value[prev] = M_value[curr];
			M_value[curr] = temp;

			temp = M_ts[prev];
			M_ts[prev] = M_ts[curr];
			M_ts[curr] = temp;
			
			curr = curr - 1;
		}
		else /* shift down */
		{
			temp = M_value[next];
			M_value[next] = M_value[curr];
			M_value[curr] = temp;

			temp = M_ts[next];
			M_ts[next] = M_ts[curr];
			M_ts[curr] = temp;
			
			curr = curr + 1;
		}
		
		prev = curr - 1;
		if(prev < 0)
			prev = 0;
		
		next = curr + 1;
		if(next > (size - 1))
			next = size - 1;
	}
	
	/*
	 * new list should be sorted
	 */
	
	return;
}


/*
 * these next two routines can do windowed medians for different
 * window sizes.  We put them in for the Adjusted versions but
 * all medians should use them
 */
double
FindMedian(double *sorted_list, 
	   double *sorted_ts, 
	   int list_size,
	   double now, 
	   int win_size)
{
	int i;
	int count;
	int temp;
	double val;
	int last_i;
	
	last_i = 0;
	
	if(win_size == 1)
	{
		return(sorted_list[0]);
	}
	
	/*
	 * sweep through sorted list counting only those that are
	 * within a win_size of now.  stop when we have win_size/2
	 */
	count = 0;
	for(i=0; i < list_size; i++)
	{
		if(sorted_ts[i] > (now - win_size))
		{
			count++;
			/*
		 	 * need to remember this one for the even case
		 	 */
			if(count == (win_size / 2))
			{
				last_i = i;
			}
		}

		if(count == ((win_size / 2) + 1))
			break;
	}
	
	/*
	 * i should now be the index of middle element -- almost
	 */
	temp = win_size / 2;
	/*
	 * if even
	 */
	if(win_size == (temp * 2))
	{
		val = (sorted_list[last_i] + sorted_list[i]) / 2.0;
	}
	else
	{
		val = sorted_list[i];
	}
	
	return(val);
}

double
FindTrimMedian(double *sorted_list, 
	   double *sorted_ts, 
	   double now, 
	   int win_size,
	   double alpha)
{
	int i;
	double val;
	int T;
	double vcount;
	
	if(win_size == 1)
	{
		return(sorted_list[0]);
	}
	
	/*
	 * sweep through sorted list counting only those that are
	 * within a win_size of now.  stop when we have win_size/2
	 */
	T = alpha * win_size;
	val = 0.0;
	vcount = 0.0;
	for(i=0; i < win_size ; i++)
	{
		if((sorted_ts[i] > (now - win_size + T)) &&
		   (sorted_ts[i] < (now - T)))
		{
			val += sorted_list[i];
			vcount++;
		}
	}
	
	if(vcount > 0.0)
	{
		val = val / vcount;
	}
	else
	{
		val = 0.0;
	}

	return(val);
}


char *
InitMedian(fbuff series, fbuff time_stamps, char *params)
{
	struct median_state *state;
	char *p_str;
	double M_size;
	int i;
	
	state = (struct median_state *)malloc(sizeof(struct median_state));
	
	if(state == NULL)
	{
		return(NULL);
	}
	
	memset((void *)state,0,sizeof(struct median_state));
	
	state->trim = 0;
	if(params != NULL)
	{
		/*
		 * first parameter is siaze of median window
		 */
		p_str = params;
		M_size = (int)strtod(p_str,&p_str);
		state->M_size = M_size;
	}
	else
	{
		state->M_size = MAX_MED_SIZE;
	}
	
	/*
	 * init M_ts to all SMALL_TS so that MSort will build sorted
	 * list correctly
	 */
	for(i=0; i < MAX_MED_SIZE; i++)
	{
		state->M_ts[i] = SMALL_TS;
	}
	state->artificial_time = 0;
	state->series = series;
	state->time_stamps = time_stamps;
	state->M_count = 0;
	
	return((char *)state);
}

void
FreeMedian(char *state)
{
	free(state);
	return;
}

void
UpdateMedian(char *state,
	     double ts,
	     double value)
{
	struct median_state *s = (struct median_state *)state;
	int curr_size;
	

	curr_size = F_COUNT(s->series);

	if(curr_size > s->M_size)
	{
		s->M_count = curr_size = s->M_size;
	}
	else
	{
		s->M_count = curr_size;
	}

	/*
	 * update the sorted list
	 */
	
	/*
	 * increment the artificial time stamp
	 */
	s->artificial_time = s->artificial_time + 1;

	/*
	 * use artificial time stamp instead of real one to
	 * keep things in terms of entries instead of seconds
	 */
	MSort(s->M_array,s->M_ts,value,s->artificial_time,curr_size);
	
	return;
}


int
ForcMedian(char *state, double *forecast)
{
	struct median_state *s = (struct median_state *)state;
	int l_index;
	double val;
	
	if(s->M_count < 1)
	{
		return(0);
	}

	l_index = s->M_count/2;

	/*
	 * if even
	 */
	if(s->M_count == (2 * l_index))
	{
		val = (s->M_array[l_index-1] + s->M_array[l_index]) / 2.0;
	}
	else
	{
		val = s->M_array[l_index];
	}
	
	*forecast = val;
	return(1);
}

char *InitTrimMedian(fbuff series, fbuff time_stamps, char *params)
{
	struct median_state *state;
	char *p_str;
	double M_size;
	double alpha;
	int i;
	
	state = (struct median_state *)malloc(sizeof(struct median_state));
	
	if(state == NULL)
	{
		return(NULL);
	}

	memset((void *)state,0,sizeof(struct median_state));
	
	state->trim = 0;
	if(params != NULL)
	{
		/*
		 * first parameter is size of median window
		 */
		p_str = params;
		M_size = (int)strtod(p_str,&p_str);
		state->M_size = M_size;
		
		/*
		 * second parameter is the alpha trim value
		 */
		alpha = strtod(p_str, &p_str);
		state->trim = alpha;
	}
	else
	{
		state->M_size = MAX_MED_SIZE;
		state->trim = 0.0;
	}
	
	/*
	 * init M_ts to all SMALL_TS so that MSort will build sorted
	 * list correctly
	 */
	for(i=0; i < MAX_MED_SIZE; i++)
	{
		state->M_ts[i] = SMALL_TS;
	}
	state->artificial_time = 0;
	state->series = series;
	state->time_stamps = time_stamps;
	state->M_count = 0;
	
	return((char *)state);
}

void
FreeTrimMedian(char *state)
{
	FreeMedian(state);
	return;
}

void
UpdateTrimMedian(char *state, double ts, double value)
{
	UpdateMedian(state,ts,value);
	return;
}



/*
 * page 264 in DSP book
 */
int
ForcTrimMedian(char *state, double *forecast)
{
	struct median_state *s = (struct median_state *)state;
	double val;
	int l_index;
	int T;
	
	if(s->M_count < 1)
	{
		return(0);
	}


	T = (int)(s->trim * s->M_count);

	val = 0.0;
	for(l_index = T; l_index < (s->M_count - T); l_index++)
	{
		val += s->M_array[l_index];
	}

	val = val / (double)(s->M_count - 2*T);


	*forecast = val;
	
	return(1);
}



char *InitAdMedian(fbuff series, fbuff time_stamps, char *params)
{
	struct ad_median_state *state;
	char *p_str;
	double win;
	double min_win;
	double max_win;
	double offset;
	int i;
	
	state = (struct ad_median_state *)
		malloc(sizeof(struct ad_median_state));
	
	if(state == NULL)
	{
		return(NULL);
	}

	memset((void *)state,0,sizeof(struct median_state));
	
	
	if(params != NULL)
	{
		/*
		 * first parameter is initial target size of median window
		 */
		p_str = params;
		win = (int)strtod(p_str,&p_str);
		state->win = win;
		
		/*
		 * defaults
		 */
		min_win = win;
		max_win = win;
		offset = 1;
		
		/*
		 * second parameter is the min window value
		 */
		min_win = strtod(p_str, &p_str);

		/*
		 * third parameter is the max window value
		 */
		max_win = strtod(p_str, &p_str);

		/*
		 * fourth parameter is the offset value to use
		 * when calculating alternative window sizes
		 */
		offset = strtod(p_str, &p_str);
		state->offset = (int)offset;
		
		/*
		 * sanity checking
		 */
		if(win == 1)
			state->offset = 0;
		
		if((win - state->offset) < 0)
			state->offset = 1;
			
		if(min_win > (win - state->offset))
			min_win = win - state->offset;
		
		if(max_win < (win + state->offset))
			max_win = win + state->offset;

		state->min = (int)min_win;
		state->max = (int)max_win;
	}
	else
	{
		state->min = state->max = state->win = MAX_MED_SIZE;
		state->offset = 1;
	}
	
	/*
	 * init M_ts to all SMALL_TS so that MSort will build sorted
	 * list correctly
	 */
	for(i=0; i < MAX_MED_SIZE; i++)
	{
		state->M_ts[i] = SMALL_TS;
	}
	state->artificial_time = 0;
	state->series = series;
	state->time_stamps = time_stamps;
	state->M_count = 0;
	state->artificial_time = 1;
	
	return((char *)state);
}

void
FreeAdMedian(char *state)
{
	        free(state);
	        return;
}

void
UpdateAdMedian(char *state, double ts, double value) 
{
	struct ad_median_state *s = (struct ad_median_state *)state;
	int curr_size;
	int win;
	double less_val;
	double eq_val;
	double more_val;
	double less_err;
	double eq_err;
	double more_err;
	int lo_offset;
	int hi_offset;
	

	curr_size = F_COUNT(s->series);

	/*
	 * M_size is the current window size, and M_count is the
	 * current amount of data in the median buffer
	 */
	
	if(curr_size > s->max)
	{
		s->M_count = curr_size = s->max;
	}
	else
	{
		s->M_count = curr_size;
	}

	/*
	 * update the sorted list
	 */
	
	/*
	 * increment the artificial time stamp
	 */
	s->artificial_time = s->artificial_time + 1;

	/*
	 * use artificial time stamp instead of real one to
	 * keep things in terms of entries instead of seconds
	 */
	MSort(s->M_array,s->M_ts,value,s->artificial_time,curr_size);
	
	
	/*
	 * calculate the window based on how much data there is
	 */
	if(curr_size > s->win)
	{
		win = s->win;
	}
	else
	{
		win = curr_size;
	}
	/*
	 * find the median using the current
	 * window size
	 */
	eq_val = FindMedian(s->M_array,
			    s->M_ts,
			    s->M_count,
			    s->artificial_time,
			    win);
	
	/*
	 * we want to wait until there is enough data before we start
	 * to adapt.  We don't start to adjust s->win until there is
	 * enough data to get out to the max window size
	 */
	if(curr_size < s->max)
	{
		return;
	}
	
	if((win - s->offset) < 0)
	{
		lo_offset = win - 1;
	}
	else
	{
		lo_offset = s->offset;
	}
	
	if((win + s->offset) > s->M_count)
	{
		hi_offset = s->M_count - win - 1;
	}
	else
	{
		hi_offset = s->offset;
	}
	
	/*
	 * find the median for a smaller window -- offset
	 * controls how much smaller or bigger the window should be
	 * that we consider
	 */
	less_val = FindMedian(s->M_array,
			      s->M_ts,
			      s->M_count,
			      s->artificial_time,
			      lo_offset);

	/*
	 * find the median for a bigger window -- offset
	 * controls how much smaller or bigger the window should be
	 * that we consider
	 */
	more_val = FindMedian(s->M_array,
			      s->M_ts,
			      s->M_count,
			      s->artificial_time,
			      hi_offset);
	
	/*
	 * now, calculate the errors
	 */
	less_err = (value - less_val) * (value - less_val);
	more_err = (value - more_val) * (value - more_val);
	eq_err = (value - eq_val) * (value - eq_val);
	
	/*
	 * adapt the window according to the direction giving us the
	 * smallest error
	 */
	if(less_err < eq_err)
	{
		if(less_err < more_err)
		{
			win = win - 1;
		}
		else if(more_err < eq_err)
		{
			win = win + 1;
		}
	}
	else if(more_err < eq_err)
	{
		if(more_err < less_err)
		{
			win = win + 1;
		}
		else if(less_err < eq_err)
		{
			win = win - 1;
		}
	}

	s->win = win;

	return;
}	
	

int
ForcAdMedian(char *state, double *forecast)
{
	struct ad_median_state *s = (struct ad_median_state *)state;
	double forc;
	
	if(s->M_count == 0)
	{
		return(0);
	}
	
	if(s->M_count < s->max)
	{
		forc = FindMedian(s->M_array,
				  s->M_ts,
				  s->M_count,
				  s->artificial_time,
				  s->M_count);
	}
	else
	{
		forc = FindMedian(s->M_array,
				  s->M_ts,
				  s->M_count,
				  s->artificial_time,
				  s->win);
		
	}
	
	*forecast = forc;
	
	return(1);
}

	
	


	



#ifdef NOTYET

AdjustedMedian(tp,pred,min,max)
struct thruput_pred *tp;
int pred;
int min;
int max;
{
	double err1;
	double err2;
	double err3;
	double val1;
	double val2;
	double val3;
	int win;
	struct value_el *data = tp->data;
	int prev = MODMINUS(tp->head,1,HISTORYSIZE);
	int head = tp->head;
	double value = tp->data[head].value;
	double predictor = PRED(pred,data[prev]);
	int out_win;
	double out_val;

	win = MedWIN(pred,data[prev]);
	if(MODMINUS(head,tp->tail,HISTORYSIZE) < 3)
	{
		PRED(pred,data[head]) = value;
		MedWIN(pred,data[head]) = win;
		return(0.0);
	}
	if(win < 2)
		win = 2;
	val1 = Median(tp,win-1);
	val2 = Median(tp,win+1);
	val3 = Median(tp,win);

	err1 = (value - val1) * (value - val1);
	err2 = (value - val2) * (value - val2);
	err3 = (value - val3) * (value - val3);

	if(err1 < err2)
	{
		if(err1 < err3)
		{
			out_win = win - 1;
			out_val = val1;
		}
		else
		{
			out_win = win;
			out_val = val3;
		}
	}
	else if(err2 < err1)
	{
		if(err2 < err3)
		{
			out_win = win + 1;
			out_val = val2;
		}
		else
		{
			out_win = win;
			out_val = val3;
		}
	}
	else
	{
		out_win = win;
		out_val = val3;
	}

	if(out_win < min)
	{
		out_win = min;
		out_val = Median(tp,min);
	}

	if(out_win > max)
	{
		out_win = max;
		out_val = Median(tp,max);
	}

	MedWIN(pred,data[head]) = out_win;
	PRED(pred,data[head]) = out_val;
}

AdjustedMean(tp,pred,min,max)
struct thruput_pred *tp;
int pred;
int min;
int max;
{
	double err1;
	double err2;
	double err3;
	double val1;
	double val2;
	double val3;
	int win;
	struct value_el *data = tp->data;
	int prev = MODMINUS(tp->head,1,HISTORYSIZE);
	int head = tp->head;
	double value = tp->data[head].value;
	double predictor = PRED(pred,data[prev]);
	int out_win;
	double out_val;

	win = MeanWIN(pred,data[prev]);
	if(MODMINUS(head,tp->tail,HISTORYSIZE) < 3)
	{
		PRED(pred,data[head]) = value;
		MeanWIN(pred,data[head]) = WIN_INIT;
		return(0.0);
	}
	if(win < 2)
		win = 2;
	val1 = TrimmedMedian(tp,win-1,0.0);
	val2 = TrimmedMedian(tp,win+1,0.0);
	val3 = TrimmedMedian(tp,win,0.0);

	err1 = (value - val1) * (value - val1);
	err2 = (value - val2) * (value - val2);
	err3 = (value - val3) * (value - val3);

	if(err1 < err2)
	{
		if(err1 < err3)
		{
			out_win = win - 1;
			out_val = val1;
		}
		else
		{
			out_win = win;
			out_val = val3;
		}
	}
	else if(err2 < err1)
	{
		if(err2 < err3)
		{
			out_win = win + 1;
			out_val = val2;
		}
		else
		{
			out_win = win;
			out_val = val3;
		}
	}
	else
	{
		out_win = win;
		out_val = val3;
	}

	if(out_win < min)
	{
		out_win = min;
		out_val = TrimmedMedian(tp,min,0.0);
	}

	if(out_win > max)
	{
		out_win = max;
		out_val = TrimmedMedian(tp,max,0.0);
	}

	MeanWIN(pred,data[head]) = out_win;
	PRED(pred,data[head]) = out_val;
}

#endif
