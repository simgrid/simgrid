#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>



/*
 * individual forecaster packages
 */
#include "forecasters.h"

#include "mse_forc.h"
#include "forc.h"

#define FORECASTER(name,init,params,freer,update,forecaster)	\
{								\
	frb = InitForcB(series,					\
			time_stamps,				\
			(name),					\
			(init),					\
			(params),				\
			(freer),				\
			(update),				\
			(forecaster));				\
								\
	if(frb == NULL)						\
	{							\
		free(l_forcl);					\
		free(forcs);					\
		free(derived_forcs);				\
		FreeFBuff(series);				\
		FreeFBuff(time_stamps);				\
		return(NULL);					\
	}							\
								\
	frb->series = series;					\
	frb->time_stamps = time_stamps;				\
	forcs[l_forcl->count] = frb;				\
	l_forcl->count = l_forcl->count + 1;			\
}


#define DERIVED_FORC(name,init,params,freer,update,forecaster) 	\
{								\
	frb = InitForcB(series,					\
		  time_stamps,					\
		  (name),					\
		  (init),					\
		  (params),					\
		  (freer),					\
		  (update),					\
		  (forecaster));				\
								\
	if(frb == NULL)						\
	{							\
		free((char *)forcs);					\
		free((char *)derived_forcs);				\
		FreeFBuff(series);				\
		FreeFBuff(time_stamps);				\
		for(i=0; i < l_forcl->count; i++)		\
		{						\
			FreeForcB(l_forcl->forcs[i]);		\
		}						\
		free((char *)l_forcl);					\
		return(NULL);					\
	}							\
	frb->series = series;					\
	frb->time_stamps = time_stamps;				\
	derived_forcs[l_forcl->derived_count] = frb;		\
	l_forcl->derived_count = l_forcl->derived_count + 1;	\
}

forcb
InitForcB(fbuff series,
	  fbuff time_stamps,
	  const char *name,
	  char *(*init)(),
	  const char *params,
	  void (*freer)(),
	  void (*update)(),
	  int (*forecast)())
{
	forcb frb;
	
	frb = (forcb)malloc(FORCB_SIZE);
	
	
	if(frb == NULL)
	{
		return(NULL);
	}
	
	/*
	 * get fbuff space for the cumulative error series -- needed
	 * for windowed MSE and MAE predictions
	 */
	frb->se_series = InitFBuff(MAX_MSE_WIN);

	if(frb->se_series == NULL)
	{
		free(frb);
		return(NULL);
	}
	
	frb->ae_series = InitFBuff(MAX_MSE_WIN);

	if(frb->se_series == NULL)
	{
		FreeFBuff(frb->se_series);
		free(frb);
		return(NULL);
	}
	/*
	 * could be NULL because of derived forecasters
	 */
	if(init != NULL)
	{
		frb->state = (*init)(series,time_stamps,params);
		if(frb == NULL)
		{
			FreeFBuff(frb->se_series);
			FreeFBuff(frb->ae_series);
			free(frb);
			fprintf(stderr,"InitForcB: init failed for %s\n",
					name);
			fflush(stderr);
			return(NULL);
		}
	}
	else
	{
		frb->state = NULL;
	}
	
	frb->update = update;
	frb->forecast = forecast;
	frb->free = freer;
	

	frb->se = 0.0;
	frb->ae = 0.0;
	frb->count = 0.0;
	strncpy(frb->name,name,sizeof(frb->name));
	
	/*
	 * point forcb at the series and time stamp buffers
	 */
	frb->series = series;
	frb->time_stamps = time_stamps;

	frb->best_f = 0.0;
	frb->best_i = 0;
	frb->best_err = 0.0;

	
	return(frb);
}

void
FreeForcB(forcb frb)
{
	if(frb->state != NULL)
	{
		frb->free(frb->state);
	}

	/*
	 * defensive programming
	 */
	if(frb->se_series != NULL)
	{
		FreeFBuff(frb->se_series);
	}
	if(frb->ae_series != NULL)
	{
		FreeFBuff(frb->ae_series);
	}
	free(frb);
	
	return;
}

void
FreeForcl(char *i_forcl)
{
	int i;
	forcl l_forcl;
	
	l_forcl = (forcl)i_forcl;

	/*
	 * shared by all forcbs
	 */
	FreeFBuff(l_forcl->forcs[0]->series);
	FreeFBuff(l_forcl->forcs[0]->time_stamps);
	
	for(i=0; i < l_forcl->count; i++)
	{
		FreeForcB(l_forcl->forcs[i]);
	}
	for(i=0; i < l_forcl->derived_count; i++)
	{
		FreeForcB(l_forcl->derived_forcs[i]);
	}

	/*
	 * the state is the l_forcl itself -- clear it to free works
	 * properly
	 */
	l_forcl->total_mse->state = NULL;
	l_forcl->total_mae->state = NULL;
	FreeForcB(l_forcl->total_mse);
	FreeForcB(l_forcl->total_mae);
	
	free(l_forcl->derived_forcs);
	free(l_forcl->forcs);
	free(l_forcl);
	
	return;
}
	
/*
 * initializes a single fbuff for the data series and another for the
 * time stamps. 
 * 
 * initializes an array of forcb structs, one per forecaster
 * 
 * points each forcb to the series and time stamps
 */
char *
InitForcl(int max_forc_count, int buff_size)
{
	forcb frb;
	forcb *forcs;
	forcb *derived_forcs;
	forcl l_forcl;
	fbuff series;
	fbuff time_stamps;
	int i;
	char derived_params[255];
	
	/*
	 * first, get space for series and time stamps
	 */
	series = InitFBuff(buff_size);
	if(series == NULL)
	{
		fprintf(stderr,"InitForcs: couldn't get fbuff for series\n");
		fflush(stderr);
		return(NULL);
	}
	
	time_stamps = InitFBuff(buff_size);
	if(time_stamps == NULL)
	{
		fprintf(stderr,
			"InitForcs: couldn't get fbuff for time stamps\n");
		fflush(stderr);
		FreeFBuff(series);
		return(NULL);
	}
	
	/*
	 * now, get forcl list space
	 */
	l_forcl = (forcl)(malloc(FORCL_SIZE));
	if(l_forcl == NULL)
	{
		fprintf(stderr,"InitForcs: couldn't malloc %d forcl\n",
				max_forc_count);
		fflush(stderr);
		FreeFBuff(series);
		FreeFBuff(time_stamps);
		return(NULL);
	}
	
	memset((char *)l_forcl,0,FORCL_SIZE);
	l_forcl->derived_count = 0;
   
	/*
	 * get space for individual forecasters
	 */
	forcs = (forcb *)(malloc(max_forc_count*sizeof(forcb)));
	
	if(forcs == NULL)
	{
		fprintf(stderr,"InitForcl: couldn't malloc %d forcb pointers\n",
				max_forc_count);
		fflush(stderr);
		free(l_forcl);
		FreeFBuff(series);
		FreeFBuff(time_stamps);
		return(NULL);
	}

	/*
	 * get space for derived forecasters
	 */
	derived_forcs = (forcb *)(malloc(max_forc_count*sizeof(forcb)));
	
	if(derived_forcs == NULL)
	{
		fprintf(stderr,"InitForcl: couldn't malloc %d derived forcb pointers\n",
				max_forc_count);
		fflush(stderr);
		free(l_forcl);
		FreeFBuff(series);
		FreeFBuff(time_stamps);
		free(forcs);
		return(NULL);
	}
	
	l_forcl->forcs = forcs;
	l_forcl->derived_forcs = derived_forcs;
	
	/*
	 * include the primary forecasters
	 */
#include "forecasters.c"


	/*
	 * now do derived forecaster types
	 */
	
	/*
	 * for min MSE, we set the window size to 0.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		0);
	DERIVED_FORC("Minimum MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);

	/*
	 * for min MAE, we set the window size to 0.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		0);

	DERIVED_FORC("Minimum MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);

	/*
	 * for min windowed MSE, we set the window size to 1.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		1);
	DERIVED_FORC("Minimum Window 1 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);
	/*
	 * for min windowed MSE, we set the window size to 5.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		5);
	DERIVED_FORC("Minimum Window 5 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);

	/*
	 * for min windowed MSE, we set the window size to 10.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		10);
	DERIVED_FORC("Minimum Window 10 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);

	/*
	 * for min windowed MSE, we set the window size to 20.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		20);
	DERIVED_FORC("Minimum Window 20 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);
	/*
	 * for min windowed MSE, we set the window size to 30.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		30);
	DERIVED_FORC("Minimum Window 30 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);
	/*
	 * for min windowed MSE, we set the window size to 50.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		50);
	DERIVED_FORC("Minimum Window 50 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);
	/*
	 * for min windowed MSE, we set the window size to 100.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		100);
	DERIVED_FORC("Minimum Window 100 MSE",
		     InitWinMSE,
		     derived_params,
		     FreeWinMSE,
		     NULL,
		     LocalWinMSEForecast);

	/*
	 * for win min MAE, with window size to 1.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		1);

	DERIVED_FORC("Minimum Window 1 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 5.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		5);

	DERIVED_FORC("Minimum Window 5 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 10.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		10);

	DERIVED_FORC("Minimum Window 10 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 20.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		20);

	DERIVED_FORC("Minimum Window 20 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 30.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		30);

	DERIVED_FORC("Minimum Window 30 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 50.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		50);

	DERIVED_FORC("Minimum Window 50 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * for win min MAE, with window size to 100.  The first
	 * argument is the address of the forc list structure we are building
	 * 
	 * thread safe?  I think not.
	 */
	sprintf(derived_params,
		"%p %d",
		(void *)l_forcl,
		100);

	DERIVED_FORC("Minimum Window 100 MAE",
		     InitWinMAE,
		     derived_params,
		     FreeWinMAE,
		     NULL,
		     LocalWinMAEForecast);
	/*
	 * end of derived forecaster init
	 */
	
	/*
	 * now init a total MSE forcb for the
	 * list
	 */
	
	frb = InitForcB(series,
			time_stamps,
			"Total MSE",
			NULL,		/* no init */
			NULL,		/* no params */
			NULL,		/* no free */
			NULL,		/* no update */
			TotalMSEForecast);	/* MSE forecast */
	if(frb == NULL)
	{
		free(forcs);
		free(derived_forcs);
		FreeFBuff(series);
		FreeFBuff(time_stamps);
		for(i=0; i < l_forcl->count; i++)
		{
			FreeForcB(l_forcl->forcs[i]);
		}
		for(i=0; i < l_forcl->derived_count; i++)
		{
			FreeForcB(l_forcl->derived_forcs[i]);
		}
		free(l_forcl->derived_forcs);
		free(l_forcl);
		return(NULL);
	}
	frb->series = series;
	frb->time_stamps = time_stamps;
	frb->ae = 0.0;
	frb->se = 0.0;
	frb->count = 0.0;
	/*
	 * total forecasters take forcl as state
	 */
	frb->state = (char *)l_forcl;
	l_forcl->total_mse = frb;

	/*
	 * now init a total MAE forcb for the
	 * list
	 */
	
	frb = InitForcB(series,
			time_stamps,
			"Total MAE",
			NULL,		/* no init */
			NULL,		/* no params */
			NULL,		/* no free */
			NULL,		/* no update */
			TotalMAEForecast);	/* MSE forecast */
	if(frb == NULL)
	{
		free(forcs);
		free(derived_forcs);
		FreeFBuff(series);
		FreeFBuff(time_stamps);
		for(i=0; i < l_forcl->count; i++)
		{
			FreeForcB(l_forcl->forcs[i]);
		}
		for(i=0; i < l_forcl->derived_count; i++)
		{
			FreeForcB(l_forcl->derived_forcs[i]);
		}
		free(l_forcl->derived_forcs);
		free(l_forcl);
		return(NULL);
	}
	frb->series = series;
	frb->time_stamps = time_stamps;
	frb->ae = 0.0;
	frb->se = 0.0;
	frb->count = 0.0;
	/*
	 * total forecasters take forcl as state
	 */
	frb->state = (char *)l_forcl;
	l_forcl->total_mae = frb;
	
	

	return((char *)l_forcl);
	
}

void
UpdateForecasts(char *i_forcl, double ts, double value)
{
	int i;
	double err;
	double forecast;
	int ferr;
	forcl l_forcl;
	int forc_okay;
	double best_sq_f;
	double best_f;
	int best_i;
	int best_sq_i;
	double best_sq_err;
	double best_err;
	
	l_forcl = (forcl)i_forcl;
	
	forc_okay = 0;
	
	/*
	 * get mse forecast for this value and update error
	 */
	ferr = (l_forcl->total_mse)->forecast((l_forcl->total_mse)->state,
			      &forecast);
	if(ferr == 1)
	{
		forc_okay = 1;
		err = forecast - value;
		if(err < 0.0)
		{
			err = err * -1.0;
		}
		l_forcl->total_mse->ae += err;
		l_forcl->total_mse->se += err*err;
		l_forcl->total_mse->count += 1.0;
		
		/*
		 * write out the cumulative total in the
		 * series buffer so that we may do windowed
		 * mse and mae things
		 */
		UpdateFBuff(l_forcl->total_mse->se_series,
			   l_forcl->total_mse->se); 
		UpdateFBuff(l_forcl->total_mse->ae_series,
			   l_forcl->total_mse->ae); 
	}
	/*
	 * get mae forecast for this value and update error
	 */
	ferr = (l_forcl->total_mae)->forecast((l_forcl->total_mae)->state,
			      &forecast);
	if(ferr == 1)
	{
		err = forecast - value;
		if(err < 0.0)
		{
			err = err * -1.0;
		}
		l_forcl->total_mae->ae += err;
		l_forcl->total_mae->se += err*err;
		l_forcl->total_mae->count += 1.0;
		
		/*
		 * write out the cumulative total in the
		 * series buffer so that we may do windowed
		 * mse and mae things
		 */
		UpdateFBuff(l_forcl->total_mae->se_series,
			   l_forcl->total_mae->se); 
		UpdateFBuff(l_forcl->total_mae->ae_series,
			   l_forcl->total_mae->ae); 
	}

	if(forc_okay == 1)
	{
		/*
		 * update derived forecast error values
		 */
		for(i=0; i < l_forcl->derived_count; i++)
		{
			/*
			 * get forecast for this value and update error
			 */
			ferr = 
	(l_forcl->derived_forcs[i])->forecast((l_forcl->derived_forcs[i])->state,
					      &forecast);
			if(ferr == 0)
			{
				continue;
			}
			forc_okay = 1;
			err = forecast - value;
			if(err < 0.0)
			{
				err = err * -1.0;
			}
			l_forcl->derived_forcs[i]->ae += err;
			l_forcl->derived_forcs[i]->se += err*err;
			l_forcl->derived_forcs[i]->count += 1.0;
			
			/*
			 * write out the cumulative total in the
			 * series buffer so that we may do windowed
			 * mse and mae things
			 */
			UpdateFBuff(l_forcl->derived_forcs[i]->se_series,
				   l_forcl->derived_forcs[i]->se); 
			UpdateFBuff(l_forcl->derived_forcs[i]->ae_series,
				   l_forcl->derived_forcs[i]->ae); 
		}
	}
	/*
	 * first, update the error values
	 */
	best_sq_err = DBIG_VAL;
	best_err = DBIG_VAL;
	best_sq_f = 0.0;
	best_f = 0.0;
	best_sq_i = 0;
	best_i = 0;
	for(i=0; i < l_forcl->count; i++)
	{
		/*
		 * get forecast for this value and update error
		 */
		ferr = 
	(l_forcl->forcs[i])->forecast((l_forcl->forcs[i])->state,&forecast);
		if(ferr == 0)
		{
			continue;
		}
		
		err = forecast - value;
		if(err < 0.0)
		{
			err = err * -1.0;
		}
		l_forcl->forcs[i]->ae += err;
		l_forcl->forcs[i]->se += err*err;
		l_forcl->forcs[i]->count += 1.0;

		/*
		 * find the forecast that actually has the min error
		 */
		if((err*err) < best_sq_err)
		{
			best_sq_err = err*err;
			best_sq_i = i;
			best_sq_f = forecast;
		}
		if(err < best_err)
		{
			best_err = err;
			best_i = i;
			best_f = forecast;
		}
		/*
		 * write out the cumulative total in the
		 * series buffer so that we may do windowed
		 * mse and mae things
		 */
		UpdateFBuff(l_forcl->forcs[i]->se_series,
			   l_forcl->forcs[i]->se); 
		UpdateFBuff(l_forcl->forcs[i]->ae_series,
			   l_forcl->forcs[i]->ae); 
	}

	/*
	 * record the forecast that actually won
	 */
	if(best_sq_err != DBIG_VAL)
	{
		l_forcl->total_mse->best_f = best_sq_f;
		l_forcl->total_mse->best_i = best_sq_i;
		l_forcl->total_mse->best_err += best_sq_err;

		l_forcl->total_mae->best_f = best_f;
		l_forcl->total_mae->best_i = best_i;
		l_forcl->total_mae->best_err += best_err;
	}
	
	/*
	 * now update the series and time_stamps fbuffs.  all forcs point
	 * the same series and time_stamp fbuffs
	 */ 
	
	if(l_forcl->count >= 1)
	{
		UpdateFBuff(l_forcl->forcs[0]->series, value);
		UpdateFBuff(l_forcl->forcs[0]->time_stamps, ts);
	}
	
	for(i=0; i < l_forcl->count; i++)
	{
		/*
		 * now update forecast state for the individual forecasters
		 * 
		 * note that the forecasters need not record the series and
		 * time_stamp points, hence the ts and value parameters
		 */
		(l_forcl->forcs[i])->update((l_forcl->forcs[i])->state,
				   ts,
				   value);
	}
	
	return;
}

int
ForcRange(char *i_forcl, double *low, double *high, int *low_i, int *high_i)
{
	forcl l_forcl;
	int i;
	double min = DBIG_VAL;
	double max = -1.0 * DBIG_VAL;
	double f;
	int err;
	
	l_forcl = (forcl)i_forcl;
	
	/*
	 * simple forecasters
	 */
	for(i=0; i < l_forcl->count; i++)
	{
		err = l_forcl->forcs[i]->forecast(l_forcl->forcs[i]->state,
						  &f);
		if(err == 0)
			continue;
		
		if(f < min)
		{
			min = f;
			*low_i = i;
		}
		
		if(f > max)
		{
			max = f;
			*high_i = i;
		}
	}
	
	if((min == DBIG_VAL) || (max == (-1.0*DBIG_VAL)))
	{
		return(0);
	}
			
	*low = min;
	*high = max;
	
	return(1);
	
}

void
PrintForecastSummary(char *i_forcl)
{
	forcl l_forcl;
	int i;
	
	l_forcl = (forcl)i_forcl;
	
	/*
	 * simple forecasters
	 */
	for(i=0; i < l_forcl->count; i++)
	{
		fprintf(stdout,"MSE: %3.4f\tMAE: %3.4f\t%s\n",
			l_forcl->forcs[i]->se/l_forcl->forcs[i]->count,
			l_forcl->forcs[i]->ae/l_forcl->forcs[i]->count,
			l_forcl->forcs[i]->name);
	}
	
	/*
	 * secondary forecasters
	 */
	for(i=0; i < l_forcl->derived_count; i++)
	{
		fprintf(stdout,"MSE: %3.4f\tMAE: %3.4f\t%s\n",
		l_forcl->derived_forcs[i]->se/l_forcl->derived_forcs[i]->count,
		l_forcl->derived_forcs[i]->ae/l_forcl->derived_forcs[i]->count,
		l_forcl->derived_forcs[i]->name);
	}
	
	/*
	 * totals
	 */
	fprintf(stdout,"MSE: %3.4f\tMAE: %3.4f\tTotal MSE\n",
			l_forcl->total_mse->se / l_forcl->total_mse->count,
			l_forcl->total_mse->ae / l_forcl->total_mse->count);
	fprintf(stdout,"MSE: %3.4f\tMAE: %3.4f\tTotal MAE\n",
			l_forcl->total_mae->se / l_forcl->total_mae->count,
			l_forcl->total_mae->ae / l_forcl->total_mae->count);
	fprintf(stdout,"MSE: %3.4f\tMAE: %3.4f\tOptimum\n",
			l_forcl->total_mse->best_err/l_forcl->total_mse->count,
			l_forcl->total_mae->best_err/l_forcl->total_mse->count);

	return;
}

void
PrintLifetimeForecastSummary(char *state)
{
	forclife flife;
	
	flife = (forclife)state;
	
	PrintForecastSummary(flife->forc_list);
	
	return;
}

/*
 * routine put in to support FORECASTAPI_MethodName which doesn't
 * take a state record
 */
void
GetForcNames(char *state, char *methodNames[], int max_size, int *out_size)
{
	forcl s;
	int i;
	
	s = (forcl)state;
	
	for(i=0; i < max_size; i++)
	{
		if(i >= s->count)
			break;
		
		methodNames[i] = s->forcs[i]->name;
	}
	
	*out_size = i;
	
	return;
}

char *
InitForcLife(int max_forc_count, int buff_size, double lifetime)
{
	char *l_forcl;
	forclife flife;
	
	/*
	 * get a forc list for the series
	 */
	l_forcl = InitForcl(max_forc_count, buff_size);
	if(l_forcl == NULL)
	{
		fprintf(stderr,"InitForcLife: no space for forc list\n");
		fflush(stderr);
		return(NULL);
	}
	
	/*
	 * now, get space for the lifetime forecasting structure
	 */
	flife = (forclife)malloc(FORCLIFE_SIZE);
	if(flife == NULL)
	{
		FreeForcl(l_forcl);
		fprintf(stderr,"InitForcLife: no space for forclife struct\n");
		fflush(stderr);
		return(NULL);
	}
	
	
	flife->lifetime = lifetime;
	flife->epoch_end = 0.0;
	flife->total = 0.0;
	flife->count = 0.0;
	flife->forc_list = l_forcl;
	
	return((char *)flife);
}

void
FreeForcLife(char *state)
{
	forclife flife;
	
	flife = (forclife)state;
	
	FreeForcl(flife->forc_list);
	free(flife);
	
	return;
}

void
UpdateForcLife(char *state, double ts, double value)
{
	forclife flife;
	double avg;
	
	flife = (forclife)state;
	
	/*
	 * if we are initializing
	 */
	if(flife->epoch_end == 0.0)
	{
		flife->epoch_end = ts + flife->lifetime;
	}
	
	if(ts < flife->epoch_end)
	{
		flife->total += value;
		flife->count += 1.0;
	}
	else
	{
		if(flife->count == 0.0)
			flife->count = 1.0;

		avg = flife->total / flife->count;
		UpdateForecasts((char *)flife->forc_list,
				  flife->epoch_end,
				  avg);
		flife->total = 0.0;
		flife->count = 0.0;
		flife->epoch_end = ts+flife->lifetime;
	}

	return;
}

double
LifetimeValue(char *state)
{
	forclife flife;
	forcl l_forcl;
	double value;
	
	flife = (forclife)state;
	l_forcl = (forcl)flife->forc_list;
	
	value = 
	   F_VAL(l_forcl->forcs[0]->series,F_FIRST(l_forcl->forcs[0]->series));
	
	return(value);
}

double
LifetimeTimestamp(char *state)
{
	forclife flife;
	forcl l_forcl;
	double ts;
	
	flife = (forclife)state;
	l_forcl = (forcl)flife->forc_list;
	
	ts = 
	   F_VAL(l_forcl->forcs[0]->time_stamps,
			   F_FIRST(l_forcl->forcs[0]->time_stamps));
	
	return(ts);
}
