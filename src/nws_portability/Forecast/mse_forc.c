#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>



#include "forc.h"

struct mse_state
{
	forcl forc_list;
	int win;
	int method;
	int last_index;		/* optimization */
	double last_forecast;	/* optimization */
};


/*
 * forward refs for derived forecaster types
 */
int TotalMSEForecast(char *state, double *forecast);
int LocalMAEForecast(char *state, double *forecast);

int DerivedMethod(char *state);

/*
 * the MSE forecast chooses the best forecast from the derived
 * forecasters, one of which is the total MSE forecast
 */
double
MSEForecast(char *i_forcl) 
{
	int i;
	int min_i;
	double min_se;
	double sq_err;
	double forecast;
	forcl l_forcl;
	int ferr;
	fbuff series;
	
	l_forcl = (forcl)i_forcl;
	
	/*
	 * first, check to see if there is any data -- the forecaster
	 * routines may be called with an empty series
	 */
	if(F_COUNT(l_forcl->forcs[0]->series) == 0)
	{
		forecast = FORE_ERROR_VAL;
		return(forecast);
	}



	min_se = DBIG_VAL;
	min_i = 0;

	/*
	 * find the forecaster that has the lowest current
	 * mean square error
	 */
	for(i=0; i < l_forcl->derived_count; i++)
	{
		series = l_forcl->derived_forcs[i]->se_series;
		if(F_COUNT(series) > 0)
		{
			sq_err = 
			F_VAL(series,F_FIRST(series)) / F_COUNT(series);
		}
		else
		{
			sq_err = DBIG_VAL;
		}

		if(sq_err < min_se)
		{
			min_i = i;
			min_se = sq_err;
		}
	}

	/*
	 * report the best
	 */
	ferr = 
(l_forcl->derived_forcs[min_i])->forecast((l_forcl->derived_forcs[min_i])->state,
				  &forecast);
	
	/*
	 * if an error has occurred, print and error and report
	 * FORE_ERROR_VAL
	 */
	if(ferr == 0)
	{
		/*
		 * since we know there is at least one data item,
		 * return the last on error
		 */
		forecast = F_VAL(l_forcl->forcs[0]->series,
				F_FIRST(l_forcl->forcs[0]->series));
		return(forecast);
	}
	
	/*
	 * update the method that was used
	 */
	l_forcl->total_mse_method = 
		DerivedMethod(l_forcl->derived_forcs[min_i]->state);

	return(forecast);
}

double
MSEError(char *state)
{
	forcl s;
	double val;
	
	s = (forcl)state;
	
	if(s->total_mse->count > 0.0)
	{
		val = s->total_mse->se / s->total_mse->count;
	}
	else
	{
		val = 0.0;
	}
	
	return(val);
}

int
MSEMethod(char *state)
{
	forcl s;
	
	s = (forcl)state;
	
	return(s->total_mse_method);
}

/*
 * for forecaster interface
 */
int
TotalMSEForecast(char *state, double *forecast)
{
	*forecast = MSEForecast(state);
	
	if(*forecast == FORE_ERROR_VAL)
	{
		return(0);
	}
	else
	{
		return(1);
	}
}

double
MAEForecast(char *i_forcl)
{
	int i;
	int min_i;
	double min_ae;
	double err;
	double forecast;
	forcl l_forcl;
	int ferr;
	fbuff series;
	
	l_forcl = (forcl)i_forcl;

	min_ae = DBIG_VAL;
	min_i = 0;

	/*
	 * find the forecaster that has the lowest current
	 * mean absolute error
	 */
	for(i=0; i < l_forcl->derived_count; i++)
	{
		series = l_forcl->derived_forcs[i]->ae_series;
		if(F_COUNT(series) > 0)
		{
			err = 
			F_VAL(series,F_FIRST(series)) / F_COUNT(series);
		}
		else
		{
			err = DBIG_VAL;
		}

		if(err < min_ae)
		{
			min_i = i;
			min_ae = err;
		}
	}

	/*
	 * report the best
	 */
	ferr = 
(l_forcl->derived_forcs[min_i])->forecast((l_forcl->derived_forcs[min_i])->state,
				  &forecast);
	
	/*
	 * if an error has occurred, print and error and report
	 * FORE_ERROR_VAL
	 */
	if(ferr == 0)
	{
		forecast = F_VAL(l_forcl->forcs[0]->series, 
				F_FIRST(l_forcl->forcs[0]->series));
	}

	/*
	 * update the method that was used
	 */
	l_forcl->total_mae_method = 
		DerivedMethod(l_forcl->derived_forcs[min_i]->state);
	return(forecast);
}

double
MAEError(char *state)
{
	forcl s;
	double val;
	
	s = (forcl)state;
	
	if(s->total_mae->count > 0.0)
	{
		val = s->total_mae->ae / s->total_mae->count;
	}
	else
	{
		val = 0.0;
	}
	
	return(val);
}

int
MAEMethod(char *state)
{
	forcl s;
	
	s = (forcl)state;
	
	return(s->total_mae_method);
}

int
TotalMAEForecast(char *state, double *forecast)
{
	*forecast = MAEForecast(state);
	
	if(*forecast == FORE_ERROR_VAL)
	{
		return(0);
	}
	else
	{
		return(1);
	}
}

char *
InitWinMSE(fbuff series, fbuff time_stamps, char *params)
{
	struct mse_state *state;
	int pcount;
	forcl l_forcl;
	int win;
	
	state = (struct mse_state *)malloc(sizeof(struct mse_state));
	
	if(state == NULL)
	{
		return(NULL);
	}
	
	/*
	 * first parameter is the address of the forcl and the
	 * second is the window size
	 */
	pcount = sscanf(params,"%p %d",
		        &l_forcl,
			&win);
	
	if(pcount != 2)
	{
		free(state);
		return(NULL);
	}
	
	state->forc_list = l_forcl;
	state->win = win;
	
	return((char *)state);
}

void
FreeWinMSE(char *state)
{
	free(state);
}

char *
InitWinMAE(fbuff series, fbuff time_stamps, char *params)
{
	return(InitWinMSE(series,time_stamps,params));
}

void
FreeWinMAE(char *state)
{
	FreeWinMSE(state);
}

int
LocalWinMSEForecast(char *state, double *out_f)
{
	struct mse_state *s;
	int i;
	int win;
	int min_i;
	double min_se;
	double sq_err;
	double forecast;
	forcl l_forcl;
	int ferr;
	double temp_err;
	int windex;
	fbuff series;
	
	s = (struct mse_state *)state;
	
	l_forcl = s->forc_list;
	win = s->win;


	min_se = DBIG_VAL;
	min_i = 0;
	

	/*
	 * find the forecaster that has the lowest current
	 * mean square error --
	 * 
	 * if the win size > 0, look only over the window
	 * using the cumulative series.  Otherwise, use the total
	 * cumulative value
	 */
	for(i=0; i < l_forcl->count; i++)
	{
		if((l_forcl->forcs[i])->count > 0.0)
		{
			series =
			(l_forcl->forcs[i])->se_series;
			
			/*
			 * make sure we only window as much
			 * as there is
			 */
			if(win > (F_COUNT(series) - 1))
				win = F_COUNT(series) - 1;

			if(win == 0)
			{
				/*
				 * get the first value which is
				 * the current cumulative total
				 */
				temp_err =
				F_VAL(series, F_FIRST(series));
				sq_err = temp_err / 
					(l_forcl->forcs[i])->count;
			}
			else
			{
				/*
				 * get the index of the end of the
				 * window
				 */
				windex = MODPLUS(F_FIRST(series),
						 win,
						 F_SIZE(series));
				/*
				 * the difference is the cumulative
				 * total over the window size
				 */
				temp_err = F_VAL(series, F_FIRST(series)) -
				       F_VAL(series,windex);
				sq_err = temp_err / win;
			}
		}
		else
		{
			sq_err = DBIG_VAL;
		}

		if(sq_err < min_se)
		{
			min_i = i;
			min_se = sq_err;
		}
	}

	/*
	 * report the best
	 */
	ferr = 
(l_forcl->forcs[min_i])->forecast((l_forcl->forcs[min_i])->state,&forecast);
	
	/*
	 * if an error has occurred, print and error and report
	 * FORE_ERROR_VAL
	 */
	if(ferr == 0)
	{
		forecast = F_VAL(l_forcl->forcs[0]->series, 
				F_FIRST(l_forcl->forcs[0]->series));

		return(0);
	}

	/*
	 * update the method that was used
	 */
	s->method = min_i;
	*out_f = forecast;
	
	return(1);
}

int
LocalWinMAEForecast(char *state, double *out_f)
{
	struct mse_state *s;
	int i;
	int win;
	int min_i;
	double min_ae;
	double err;
	double forecast;
	forcl l_forcl;
	int ferr;
	double temp_err;
	int windex;
	fbuff series;
	
	s = (struct mse_state *)state;
	
	l_forcl = s->forc_list;
	win = s->win;


	min_ae = DBIG_VAL;
	min_i = 0;

	/*
	 * find the forecaster that has the lowest current
	 * mean square error --
	 * 
	 * if the win size > 0, look only over the window
	 * using the cumulative series.  Otherwise, use the total
	 * cumulative value
	 */
	for(i=0; i < l_forcl->count; i++)
	{
		if((l_forcl->forcs[i])->count > 0.0)
		{
			series =
			(l_forcl->forcs[i])->ae_series;
			
			/*
			 * make sure we only window as much
			 * as there is
			 */
			if(win > (F_COUNT(series) - 1))
				win = F_COUNT(series) - 1;

			if(win == 0)
			{
				/*
				 * get the first value which is
				 * the current cumulative total
				 */
				temp_err =
				F_VAL(series, F_FIRST(series));
				err = temp_err / 
					(l_forcl->forcs[i])->count;
			}
			else
			{
				/*
				 * get the index of the end of the
				 * window
				 */
				windex = MODPLUS(F_FIRST(series),
						 win,
						 F_SIZE(series));
				/*
				 * the difference is the cumulative
				 * total over the window size
				 */
				temp_err =
					F_VAL(series, F_FIRST(series)) - 
					F_VAL(series,windex);
				err = temp_err / win;
			}
		}
		else
		{
			err = DBIG_VAL;
		}

		if(err < min_ae)
		{
			min_i = i;
			min_ae = err;
		}
	}

	/*
	 * report the best
	 */
	ferr = 
(l_forcl->forcs[min_i])->forecast((l_forcl->forcs[min_i])->state,&forecast);
	
	/*
	 * if an error has occurred, print and error and report
	 * FORE_ERROR_VAL
	 */
	if(ferr == 0)
	{
		forecast = F_VAL(l_forcl->forcs[0]->series, 
				F_FIRST(l_forcl->forcs[0]->series));

		*out_f = forecast;
		return(0);
	}

	/*
	 * update the method that was used
	 */
	s->method = min_i;
	*out_f = forecast;
	
	return(1);
}

int
DerivedMethod(char *state)
{
	struct mse_state *s;
	
	s = (struct mse_state *)state;
	
	return(s->method);
}

/*
 * lifetime versions
 */
double
MSELifetimeForecast(char *state)
{
	forclife flife;
	forcl l_forcl;
	
	flife = (forclife)state;
	l_forcl = (forcl)flife->forc_list;
	if(F_COUNT(l_forcl->forcs[0]->series) == 0)
	{
		if(flife->count == 0.0)
			return(0.0);
		return(flife->total / flife->count);
	}
	return(MSEForecast((char *)flife->forc_list));
}

double
MSELifetimeError(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(MSEError((char *)flife->forc_list));
}

int
MSELifetimeMethod(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(MSEMethod((char *)flife->forc_list));
}

double
MSELifetimeEpoch(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(flife->epoch_end);
}

double
MAELifetimeForecast(char *state)
{
	forclife flife;
	forcl l_forcl;
	
	flife = (forclife)state;
	l_forcl = (forcl)flife->forc_list;

	if(F_COUNT(l_forcl->forcs[0]->series) == 0)
	{
		if(flife->count == 0.0)
			return(0.0);
		return(flife->total / flife->count);
	}
	return(MAEForecast((char *)flife->forc_list));
}

double
MAELifetimeError(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(MAEError((char *)flife->forc_list));
}

int
MAELifetimeMethod(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(MAEMethod((char *)flife->forc_list));
}

double
MAELifetimeEpoch(char *state)
{
	forclife flife;
	flife = (forclife)state;
	return(flife->epoch_end);
}
