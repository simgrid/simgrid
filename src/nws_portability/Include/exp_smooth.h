/* $Id$ */

#if !defined(EXP_SMOOTH_H)
#define EXP_SMOOTH_H

#include "fbuff.h"

extern char *InitExpSmooth(fbuff series,
			   fbuff time_stamps,
			   char *params);

extern void FreeExpSmooth(char *state);

extern void UpdateExpSmooth(char *state,
			    double ts,
			    double value);

extern int ForcExpSmooth(char *state, double *v);
#endif

