/* $Id$ */

#include <stdlib.h>        /* free() malloc() */
#include <string.h>        /* strdup() */
#include "forc.h"     	   /* forecaster functions */
#include "diagnostic.h"    /* FAIL() */
#include "forecast_api.h"


struct FORECASTAPI_ForecastStateStruct {
  char *Forecaster_state;
};


void
FORECASTAPI_FreeForecastState(FORECASTAPI_ForecastState **state) {
  FreeForcl((*state)->Forecaster_state);
  free(*state);
  *state = NULL;
}

const char *
FORECASTAPI_MethodName(unsigned int methodIndex) {

  int i;
  static int methodCount = 0;
  static char *methodNames[MAX_FORC];
  char *stateForNames;

  if(methodCount == 0) {
    stateForNames = InitForcl(MAX_FORC, MAX_DATA_ENTRIES);
    GetForcNames(stateForNames, methodNames, MAX_FORC, &methodCount);
    /*
    ** GetForNames() gives us pointers which will be freed by FreeForcl(), so
    ** we have to strdup() them.
    */
    for(i = 0; i < methodCount; i++) {
      methodNames[i] = strdup(methodNames[i]);
    }
    FreeForcl(stateForNames);
  }

  return((methodIndex < methodCount) ? methodNames[methodIndex] : NULL);

}
  

FORECASTAPI_ForecastState *
FORECASTAPI_NewForecastState(void) {
  FORECASTAPI_ForecastState *returnValue;
  /*
   * init the forecaster state
   */
  returnValue = 
    (FORECASTAPI_ForecastState *)malloc(sizeof(FORECASTAPI_ForecastState)); 
  if(returnValue == NULL)
    FAIL("NewForecastState: out of memory\n");
  
  returnValue->Forecaster_state = InitForcl(MAX_FORC,MAX_DATA_ENTRIES);
  if(returnValue->Forecaster_state == NULL) {
    free(returnValue);
    FAIL("NewForecastState: out of memory\n");
  }
  return returnValue;
}


void
FORECASTAPI_UpdateForecastState(FORECASTAPI_ForecastState *state,
                                const FORECASTAPI_Measurement *measurements,
                                size_t howManyMeasurements,
                                FORECASTAPI_ForecastCollection *forecasts,
                                size_t howManyForecasts) {
	int i;

	/* sanity check */
	if (state == NULL) {
		return;
	}

	/* check if we want forecast */
	if (forecasts == NULL) {
		howManyForecasts = 0;
	}

	for(i = howManyMeasurements - 1; i >= 0; i--) {
		UpdateForecasts(state->Forecaster_state, (double)measurements[i].timeStamp, (double)measurements[i].measurement);
		if(i < howManyForecasts) {
			FORECASTAPI_ComputeForecast(state, &forecasts[i]);
			forecasts[i].measurement = measurements[i];
		}
	}
}

int
FORECASTAPI_ComputeForecast(        FORECASTAPI_ForecastState *state,
				FORECASTAPI_ForecastCollection *forecast) {

	/* sanity check */
	if (state == NULL || forecast == NULL) {
		return 1;
	}

	forecast->forecasts[FORECASTAPI_MAE_FORECAST].forecast = MAEForecast(state->Forecaster_state);
	forecast->forecasts[FORECASTAPI_MAE_FORECAST].error = MAEError(state->Forecaster_state);
	forecast->forecasts[FORECASTAPI_MAE_FORECAST].methodUsed = MAEMethod(state->Forecaster_state);
	forecast->forecasts[FORECASTAPI_MSE_FORECAST].forecast = MSEForecast(state->Forecaster_state);
	forecast->forecasts[FORECASTAPI_MSE_FORECAST].error = MSEError(state->Forecaster_state);
	forecast->forecasts[FORECASTAPI_MSE_FORECAST].methodUsed = MSEMethod(state->Forecaster_state);

	return 0;
}

