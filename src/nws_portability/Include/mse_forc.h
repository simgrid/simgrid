#ifndef MSE_FORC_H

#define MSE_FORC_H

#define MAX_MSE_WIN (100)

/*
 * API for forc.c
 */
int TotalMSEForecast(char *state, double *forecast);
int TotalMAEForecast(char *state, double *forecast);

char *InitWinMSE(fbuff series, fbuff time_stamps, char *params);
char *InitWinMAE(fbuff series, fbuff time_stamps, char *params);
void FreeWinMSE(char *state);
void FreeWinMAE(char *state);
int LocalWinMSEForecast(char *state, double *forecast);
int LocalWinMAEForecast(char *state, double *forecast);

#endif
