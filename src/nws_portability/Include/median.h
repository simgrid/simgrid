/* $Id$ */

#ifndef MEDIAN_H
#define MEDIAN_H

#define MAX_MED_SIZE 100

#include "fbuff.h"

char *InitMedian(fbuff series, fbuff time_stamps, char *params);
void FreeMedian(char *state);
void UpdateMedian(char *state, double ts, double value);
int ForcMedian(char *state, double *forecast);

char *InitTrimMedian(fbuff series, fbuff time_stamps, char *params);
void FreeTrimMedian(char *state);
void UpdateTrimMedian(char *state, double ts, double value);
int ForcTrimMedian(char *state, double *forecast);

char *InitAdMedian(fbuff series, fbuff time_stamps, char *params);
void FreeAdMedian(char *state);
void UpdateAdMedian(char *state, double ts, double value);
int ForcAdMedian(char *state, double *forecast);

#endif

