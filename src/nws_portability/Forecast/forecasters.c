/* $Id$ */

/*
 * Forecaster Configuration File
 * 
 * FORECASTER(name,init,params,free,update,forecaster)
 * 	name -- a print string to be associated with the forecaster
 * 	init -- initialization function
 * 	params -- string to be passed to init as initial parameters
 * 	free -- free routine to be called to release forecaster spec. state
 * 	update -- update routine for forecaster
 * 	forecaster -- forecaster routine
 * 	
 * 	To add a forecaster to the system, both a FORECASTER specifier and
 * 	an include file need to be specified.  forecasters.h contains include
 * 	files for each forecasting package.
 * 	
 * 	Each forecaster consists of a (char *)init() routine, a (void)free()
 * 	routine, a (void)update() routine, and an (int)forecaster()
 * 	routine.
 * 	
 * 	char * Init(fbuff series,
 * 	            fbuff time_stamps,
 * 	            char *params)
 * 	            
 * 		series -- a pointer to the actual data series 
 * 		time_stamps -- a pointer to the time stamp series
 * 		params -- string passed as initial parameters
 * 		
 * 		returns a local state record that will be passed to
 * 			all subsequent calls
 * 			
 * 	void Free(char *state)
 * 	
 * 		state -- local state to be deallocated when forecaster]
 * 			 is shut down
 * 			 
 * 	void Update(char *state,
 * 	            double ts,
 * 	            doubel value)
 * 	            
 * 	        state -- local forecaster state
 * 	        ts -- time stamp for new value to be added
 * 	        value -- value to be added to the forecaster state
 * 	        
 * 	int Forecaster(char *state,
 * 		       double *forecast)
 * 		       
 * 		state -- local forecaster state
 * 		forecast -- out parameter through which forecast is passed
 * 		
 * 		returns a 1 if forecast can be made and a 0 if the
 * 			foreacster fails for some reason
 * 			
 * a header file for each package shoould include external definitions
 * for each routine
 */

/*
 * last value predictor
 */
FORECASTER("Last Value", 
	    InitLastValue, 
	    NULL, 
	    FreeLastValue, 
	    UpdateLastValue, 
	    ForcLastValue);

/*
 * running tabulation of the mean
 */
FORECASTER("Running Mean", 
	    InitRunMean, 
	    NULL, 
	    FreeRunMean, 
	    UpdateRunMean, 
	    ForcRunMean);

/*
 * exp smooth predictor with 0.05 gain
 */
FORECASTER("5% Exp Smooth",
	   InitExpSmooth,
	   "0.05",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.10 gain
 */
FORECASTER("10% Exp Smooth",
	   InitExpSmooth,
	   "0.10",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.15 gain
 */
FORECASTER("15% Exp Smooth",
	   InitExpSmooth,
	   "0.15",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.20 gain
 */
FORECASTER("20% Exp Smooth",
	   InitExpSmooth,
	   "0.20",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.30 gain
 */
FORECASTER("30% Exp Smooth",
	   InitExpSmooth,
	   "0.30",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.40 gain
 */
FORECASTER("40% Exp Smooth",
	   InitExpSmooth,
	   "0.40",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.50 gain
 */
FORECASTER("50% Exp Smooth",
	   InitExpSmooth,
	   "0.50",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * exp smooth predictor with 0.75 gain
 */
FORECASTER("75% Exp Smooth",
	   InitExpSmooth,
	   "0.75",
	   FreeExpSmooth,
	   UpdateExpSmooth,
	   ForcExpSmooth);

/*
 * median predictor with window = 31
 */
FORECASTER("Median Window 31",
	   InitMedian,
	   "31.0",
	   FreeMedian,
	   UpdateMedian,
	   ForcMedian);

/*
 * median predictor with window = 5
 */
FORECASTER("Median Window 5",
	   InitMedian,
	   "5.0",
	   FreeMedian,
	   UpdateMedian,
	   ForcMedian);

/*
 * sliding window with window = 31 -- Trim Median with alpha = 0.0
 */
FORECASTER("Sliding Median Window 31",
	   InitTrimMedian,
	   "31.0 0.0",
	   FreeTrimMedian,
	   UpdateTrimMedian,
	   ForcTrimMedian);

/*
 * sliding window with window = 5 -- Trim Median with alpha = 0.0
 */
FORECASTER("Sliding Median Window 5",
	   InitTrimMedian,
	   "5.0 0.0",
	   FreeTrimMedian,
	   UpdateTrimMedian,
	   ForcTrimMedian);

/*
 * trimmed median with window = 31, alpha = 0.30
 */
FORECASTER("30% Trimmed Median Window 31",
	   InitTrimMedian,
	   "31.0 0.30",
	   FreeTrimMedian,
	   UpdateTrimMedian,
	   ForcTrimMedian);

/*
 * trimmed median with window = 51, alpha = 0.30
 */
FORECASTER("30% Trimmed Median Window 51",
	   InitTrimMedian,
	   "51.0 0.30",
	   FreeTrimMedian,
	   UpdateTrimMedian,
	   ForcTrimMedian);

/*
 * adaptive median with win=10, min_win=5, max_win=21, and a adjustment
 * value of 1
 */
FORECASTER("Adaptive Median Window 5-21",
	   InitAdMedian,
	   "10.0 5.0 21.0 1.0",
	   FreeAdMedian,
	   UpdateAdMedian,
	   ForcAdMedian);

/*
 * adaptive median with win=30, min_win=21, max_win=51, and a adjustment
 * value of 5
 */
FORECASTER("Adaptive Median Window 21-51",
	   InitAdMedian,
	   "30.0 21.0 51.0 5.0",
	   FreeAdMedian,
	   UpdateAdMedian,
	   ForcAdMedian);
