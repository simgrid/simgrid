#if !defined(FORC_H)

#define FORC_H

/*
 * forcb structure contains generic forecasting fields.  The individual
 * forecastres can see the current time series and time_stamps as well
 * as any other local state that wish to preserve.
 */

#include "fbuff.h"
#include "mse_forc.h"

struct forcb_stc
{
	fbuff series;			/* data series */
	fbuff time_stamps;		/* time stamp series */
	fbuff se_series;		/* cumulative sq. err. series */
	fbuff ae_series;		/* cumulative ab err series */
	double best_f;			/* best forecast */
	int best_i;			/* best forecast number */
	double best_err;		/* cum best err */
	char *state;
	double se;
	double ae;
	double count;
	void (*update)();
	int (*forecast)();
	void (*free)();
	char name[255];
};

typedef struct forcb_stc *forcb;

#define FORCB_SIZE (sizeof(struct forcb_stc))


/*
 * list of forcb structs -- used as a cookie through the interface
 */
struct fork_list_stc
{
	forcb *forcs;
	int count;
	forcb *derived_forcs;
	int derived_count;
	forcb total_mse;
	int total_mse_method;
	forcb total_mae;
	int total_mae_method;
};

#define FORCL_SIZE (sizeof(struct fork_list_stc))

typedef struct fork_list_stc *forcl;

struct forc_life_stc
{
	char *forc_list;
	double lifetime;
	double epoch_end;
	double total;
	double count;
};
#define FORCLIFE_SIZE (sizeof(struct forc_life_stc))

typedef struct forc_life_stc *forclife;


/*
 * initializes forecasting structure with as many as #max_forc_count#
 * forecasters and a circular buffer with #buff_size# values and
 * time_stamp entries.  Returns a cookie to be passed subsequently
 */
char *
InitForcl(int max_forc_count, int buff_size);

/*
 * lifetime version 
 */
char *
InitForcLife(int max_forc_count, int buff_size, double lifetime);

/*
 * frees forecaster state associated with cookie #i_forcl#
 */
void
FreeForcl(char *cookie);

/*
 * lifetime version
 */
void
FreeForcLife(char *state);


/*
 * updates forecaster state associated with #cookie# using #value#
 * and #time_stamp#.  Neither #value# nore #time_stamp# are checked
 * for validity
 */
void
UpdateForecasts(char *cookie, double time_stamp, double value);

/*
 * lifetime version
 */
void
UpdateForcLife(char *state, double ts, double value);

/*
 * returns value and lifetime from series (may be averages)
 */
double
LifetimeValue(char *state);

double
LifetimeTimestamp(char *state);



/*
 * generates MSE nd MAE forecasts value for data associated with #cookie#
 */
double MSEForecast(char *cookie);
double MSEOptForecast(char *cookie);
double MSEError(char *cookie);
int MSEMethod(char *cookie);
int MSEOptMethod(char *cookie);
double MAEForecast(char *cookie);
double MAEOptForecast(char *cookie);
double MAEError(char *cookie);
int MAEMethod(char *cookie);
int MAEOptMethod(char *cookie);

/*
 * same API for versions that take a lifetime and forecast the average
 */
double MSELifetimeForecast(char *cookie);
double MSELifetimeError(char *cookie);
int MSELifetimeMethod(char *cookie);
double MSELifetimeEpoch(char *cookie);
double MAELifetimeForecast(char *cookie);
double MAELifetimeError(char *cookie);
int MAELifetimeMethod(char *cookie);
double MAELifetimeEpoch(char *cookie);

/*
 * prints the low and high forecasts and their forecast numbers
 */
int
ForcRange(char *cookie, double *low, double *high, int *low_i, int *high_i);

/*
 * prints a summary of forecaster state
 */
void PrintForecastSummary(char *state);
void PrintLifetimeForecastSummary(char *state);

/*
 * routine put in to support FORECASTAPI_MethodName which doesn't
 * take a state record
 */
void GetForcNames(char *state, 
		  char *methodNames[], 
		  int max_size, 
		  int *out_size);

/*
 * generic interface requiring no size parameterization -- could waste
 * space
 */

/*
 * maximum number of forecasters
 */
#define MAX_FORC (35)

/*
 * default size of circular buffer
 */
#define MAX_DATA_ENTRIES (100)

#define INITFORECASTER() (InitForcl(MAX_FORC,MAX_DATA_ENTRIES))
#define FREEFORECASTER(cookie) (FreeForcl(cookie))
#define UPDATEFORECASTER(cookie,ts,v) (UpdateForecasts((cookie),(ts),(v)))
#define FORECAST(cookie) (MSEForecast((cookie)))

#define DBIG_VAL (999999999999999999999999999.99)
#define FORE_ERROR_VAL (DBIG_VAL)


#endif
