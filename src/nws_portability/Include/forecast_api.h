/* $Id$ */

#ifndef FORECAST_API_H
#define FORECAST_API_H


#include <sys/types.h>  /* size_t */


/*
 * This file defines the API for the Network Weather Service forecast-
 * generation library.  The functions defined here allow programs to compute
 * forecasts from time-stamp/value pairs. The logic of operation is: 
 * 	- create a new ForecasterState
 * 	- Update the state with values you observe
 * 	- get a forecast back
 *
 * NOTE: In order to avoid name collisions, all names defined here begin with
 * FORECASTAPI_.  To avoid having to use this prefix, #define
 * FORECASTAPI_SHORTNAMES before #include'ing this file.
 */


/*
 * A description of a measurement.  The time the measurement was taken
 * (represented as a number of seconds since midnight, 1/1/1970 GMT) and the
 * observed value.
 */
typedef struct {
  double timeStamp;
  double measurement;
} FORECASTAPI_Measurement;


/*
 * A description of a forecast.  The forecast value itself, an estimate of the
 * precision of the forecast, and an index for the method used to generate it.
 */
typedef struct {
  double forecast;
  double error;
  unsigned int methodUsed;
} FORECASTAPI_Forecast;


/**
 * A description of a collection of forecasts, each one based on minimizing a
 * different measurement of the error (MAE == MEAN_ABSOLUTE_ERROR, MSE ==
 * MEAN_SQUARE_ERROR).  The actual measurement taken at the same time is
 * included for convenience when available.
 */
#define FORECASTAPI_FORECAST_TYPE_COUNT 2
#define FORECASTAPI_MAE_FORECAST 0
#define FORECASTAPI_MSE_FORECAST 1
typedef struct {
	FORECASTAPI_Measurement measurement;
	FORECASTAPI_Forecast forecasts[FORECASTAPI_FORECAST_TYPE_COUNT];
} FORECASTAPI_ForecastCollection;


/**
 * A description of a forecasting state.  Client code should obtain these
 * structures via NewForecastState(), update them with new values via
 * UpdateForecastState(), then delete them via FreeForecastState().
 */
struct FORECASTAPI_ForecastStateStruct;
typedef struct FORECASTAPI_ForecastStateStruct FORECASTAPI_ForecastState;


/**
 * Frees the memory occupied by the forecasting state pointed to by #state#
 * and sets #state# to NULL.
 */
void
FORECASTAPI_FreeForecastState(FORECASTAPI_ForecastState **state);


/**
 * Returns the name of the #methodIndex#th forecasting method used by the
 * forecasting library, or NULL if #methodIndex# is out of range.
 */
const char *
FORECASTAPI_MethodName(unsigned int methodIndex);


/**
 * Allocates and returns a new forecasting state, or NULL if memory is
 * exhausted.  The caller should eventually call FreeForecastingState() to
 * free the memory.
 */
FORECASTAPI_ForecastState *
FORECASTAPI_NewForecastState(void);


/**
 * Incorporates the #howManyMeasurements#-long series #measurements# into the
 * forecasting state #state#.  If #forecasts# is non-NULL, it points to a
 * #howManyForecasts#-long array into which the function should store
 * forecasts generated during the course of the update  If #howManyForecasts#
 * is less than #howManyMeasurements#, the forecasts derived from the later
 * measurements (which appear at the beginning of #measurements#) are stored.
 */
void
FORECASTAPI_UpdateForecastState(FORECASTAPI_ForecastState *state,
                                const FORECASTAPI_Measurement *measurements,
                                size_t howManyMeasurements,
                                FORECASTAPI_ForecastCollection *forecasts,
                                size_t howManyForecasts);
#define FORECASTAPI_UpdateForecast(state, measurements, howMany) \
	FORECASTAPI_UpdateForecastState(state, measurements, howMany, NULL, 0)

/**
 * Return a set of forecast base on the current state of the forecaster.
 * #forecast# need to have space for at least one ForecastCollection.
 *
 * Return 1 in case of error, 0 otherwise.
 *
 * Warning: The measurement insite the Collection is not meaningful.
 */
int
FORECASTAPI_ComputeForecast(	FORECASTAPI_ForecastState *state,
				FORECASTAPI_ForecastCollection *forecast);

#ifdef FORECASTAPI_SHORTNAMES

#define Measurement FORECASTAPI_Measurement
#define Forecast FORECASTAPI_Forecast
#define FORECAST_TYPE_COUNT FORECASTAPI_FORECAST_TYPE_COUNT
#define MAE_FORECAST FORECASTAPI_MAE_FORECAST
#define MSE_FORECAST FORECASTAPI_MSE_FORECAST
#define ForecastCollection FORECASTAPI_ForecastCollection
#define ForecastStateStruct FORECASTAPI_ForecastStateStruct
#define ForecastState FORECASTAPI_ForecastState
#define FreeForecastState FORECASTAPI_FreeForecastState
#define MethodName FORECASTAPI_MethodName
#define NewForecastState FORECASTAPI_NewForecastState
#define UpdateForecastState FORECASTAPI_UpdateForecastState
#define UpdateForecast FORECASTAPI_UpdateForecast
#define ComputeForecast FORECASTAPI_ComputeForecast

#endif

#endif
