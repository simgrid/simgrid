/* $Id$ */

#include "config_portability.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "timeouts.h"
#include "diagnostic.h"
#include "forecast_api.h"
#include "osutil.h"

/* the default MAX timeout */
#define DEFAULT_MAX_TIMEOUT (120.0)
#define DEFAULT_MIN_TIMEOUT (5.0)

/* we have this number of entries */
#define HOW_MANY_ENTRIES (1024)

/* keep track of the measurements we got */
typedef struct {
	IPAddress addr;
	FORECASTAPI_ForecastState *state[TIMEOUT_TYPES_NUMBER];
} timeOutStruct;

static void *lock = NULL;			/* local mutex */

/* we have few types to keep track of */
static timeOutStruct timeOuts[HOW_MANY_ENTRIES]; 

/* keep track of the current limits on the timeouts */
static double to_limits[TIMEOUT_TYPES_NUMBER*2];

/* helper to work with modulo HOW_MANY_ENTRIES */
#define MODPLUS(a,b,m) (((a) + (b)) % (m))
#define MODMINUS(a,b,m) (((a) - (b) + (m)) % (m))

/*
 * Initialize the Hash structure 
 */
static void 
InitHashStructure() {
	unsigned int i,j;
	static int initialized = 0;

	/* let's do it only once */
	if (initialized != 0) {
		return;
	}

	GetNWSLock(&lock);
	initialized = 1;

	for (i=0; i < HOW_MANY_ENTRIES; i++) {
		timeOuts[i].addr = 0;
		for (j=0; j < TIMEOUT_TYPES_NUMBER; j++) {
			timeOuts[i].state[j] = NULL;
		}
	}
	ReleaseNWSLock(&lock);

	for (j=0; j < TIMEOUT_TYPES_NUMBER; j++) {
		/* set defaults to sane state */
		SetDefaultTimeout(j, DEFAULT_MIN_TIMEOUT, DEFAULT_MAX_TIMEOUT);
	}
}

/* we use a simple hash table structure to speed up the access to the
 * timeout structures. If #addr# is not in the table it is added and if
 * the table is full, space is made for the addr.  
 * Returns	# the index at which addr is been found
 */
static int 
HashIndex(IPAddress addr) {
	unsigned int i, end;

	/* initialize the structure */
	InitHashStructure();

	i = addr % HOW_MANY_ENTRIES;
	end = MODMINUS(i, 1, HOW_MANY_ENTRIES);

	GetNWSLock(&lock);
	for (; i != end; i = MODPLUS(i, 1, HOW_MANY_ENTRIES)) {
		if ((timeOuts[i].addr == addr) || (timeOuts[i].addr == 0)) {
			/* either we found it, or emtpy slot: is good */
			break;
		}
			
	}
	if (i == end) {
		/* table is full: emtpying one slot */
		i = addr % HOW_MANY_ENTRIES;
		timeOuts[i].addr = 0;
	}

	/* if this is the first time we have the item, get it to a sane
	 * state */
	if (timeOuts[i].addr == 0) {
		timeOuts[i].addr = addr;
		for (end=0; end < TIMEOUT_TYPES_NUMBER; end++) {
			/* Initialize the forecaster state */
			if (timeOuts[i].state[end] != NULL) {
				FORECASTAPI_FreeForecastState(&timeOuts[i].state[end]);
			}
			timeOuts[i].state[end] = FORECASTAPI_NewForecastState();
		}
	}

	ReleaseNWSLock(&lock);

	return i;
}

void
SetDefaultTimeout(timeoutTypes type, double min, double max) {
	/* sanity check */
	if (type < RECV || type > USER3) {
		WARN("SetDefaultTimeout: unknown type\n");
		return;
	}
	if (min < 0) {
		min = 1;
	}
	if (max < min) {
		max = 2*min;
	}

	GetNWSLock(&lock);
	to_limits[type*2] = min;
	to_limits[type*2 + 1] = max;
	ReleaseNWSLock(&lock);
}

void 
GetDefaultTimeout(timeoutTypes type, double *min, double *max) {
	/* sanity check */
	if (type < RECV || type > USER3) {
		WARN("SetDefaultTimeout: unknown type\n");
		return;
	}
	GetNWSLock(&lock);
	*min = to_limits[type*2];
	*max = to_limits[type*2 + 1];
	ReleaseNWSLock(&lock);
}


double 
GetTimeOut(timeoutTypes type, IPAddress addr, long size) {
	unsigned int i = -1;
	double ret;
	FORECASTAPI_ForecastCollection forecast;

	/* sanity check */
	if (type < 0 || type > USER3) {
		WARN("GetTimeOut: type out of range\n");
		return 0;
	}

	/* if addr is 0 (for example if it's a pipe) we pick the minimums
	 * timeout */
	if (addr == 0) {
		return to_limits[type*2];
	}

	i = HashIndex(addr);

	/* let's get a forecast */
	GetNWSLock(&lock);
	if (FORECASTAPI_ComputeForecast(timeOuts[i].state[type], &forecast) != 0) {
		forecast.forecasts[type].forecast = to_limits[type*2 + 1];
	}
	ReleaseNWSLock(&lock);

	/* let's get 3 standard deviations (if we have sqrt) */
#ifdef HAVE_SQRT
	ret = forecast.forecasts[0].forecast + 3 * sqrt(forecast.forecasts[FORECASTAPI_MSE_FORECAST].error);
#else
	ret = forecast.forecasts[0].forecast + 2 * forecast.forecasts[FORECASTAPI_MAE_FORECAST].error;
#endif

	/* adjust for the size of the packet */
	if (size > 0) {
		ret = ret * size;
	}

	if (ret > to_limits[type*2 + 1]) {
		ret = to_limits[type*2 + 1];
	} else if (ret < to_limits[type*2]) {
		ret = to_limits[type*2];
	}

	return ret;
}

void
SetTimeOut(timeoutTypes type, IPAddress addr, double duration, long size, int timedOut) {
	unsigned int i;
	FORECASTAPI_Measurement m;

	/* sanity check */
	if (type < 0 || type > USER3) {
		WARN("SetTimeOut: type out of range\n");
		return;
	}
	if (duration < 0) {
		WARN("SetTimeOut: duration negative?\n");
		return;
	}

	/* if addr is 0 (for example if it's a pipe) we return */
	if (addr == 0) {
		return;
	}

	i = HashIndex(addr);

	m.timeStamp = CurrentTime();
	/* adjust for the size of the packet */
	if (size > 0) {
		m.measurement = duration / size;
	} else {
		m.measurement = duration;
	}

	/* adjustments if we timed out before */
	if (timedOut) {
		m.measurement += 5;
	}
	GetNWSLock(&lock);
	FORECASTAPI_UpdateForecast(timeOuts[i].state[type], &m, 1);
	ReleaseNWSLock(&lock);
}
