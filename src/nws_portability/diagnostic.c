/* $Id$ */

#include "config_portability.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "diagnostic.h"
#include "osutil.h"
#include "strutil.h"

#define MAXRECORDS 2500

static FILE *diagDestinations[] =
	{DIAGSUPPRESS, DIAGSUPPRESS, DIAGSUPPRESS,
	DIAGSUPPRESS, DIAGSUPPRESS, DIAGSUPPRESS};
static long recordCounts[] =
	{0, 0, 0, 0, 0, 0};
static void *lock = NULL;			/* mutex for this modules */

/* we look quite extensively because I'm not quite sure what fprintf will
 * do if the FILE* changes on the way */
static void
PrintDiagnostic(DiagLevels level,
                const char *message,
                va_list arguments) {

	static const char *level_tags[] = {"", "", "Warning: ", "Error: ", "Fatal: ", "Debug: " };

	/* we need a lock because we write recordCounts and we use
	 * diagDestinations[] */
	if (GetNWSLock(&lock) == 0) {
		fprintf(stderr, "PrintDiagnostic: Error: Couldn't obtain the lock\n");
	}
	if(diagDestinations[level] != DIAGSUPPRESS) {
		if( (recordCounts[level]++ >= MAXRECORDS) &&
				(diagDestinations[level] != stdout) &&
				(diagDestinations[level] != stderr) ) {
		/* We want to avoid filling up the disk space when the
		 * system is running for weeks at a time.  It might be
		 * nice to save the old file under another name (maybe in
		 * /tmp), then reopen.  That requires changing the
		 * DirectDiagnostics() interface to take a file name
		 * instead of a FILE *.  */
			rewind(diagDestinations[level]);
			recordCounts[level] = 0;
		}

		fprintf(diagDestinations[level], "%.0f %d ", CurrentTime(), (int)getpid());
		fprintf(diagDestinations[level], level_tags[level]);
		vfprintf(diagDestinations[level], message, arguments);
		fflush(diagDestinations[level]);
	}
	if (ReleaseNWSLock(&lock) == 0) {
		fprintf(stderr, "PrintDiagnostic: Error: Couldn't release the lock\n");
	}
}


void
DirectDiagnostics(DiagLevels level,
                  FILE *whereTo) {

#ifdef HAVE_FILENO
	int f = 0;

	/* just an extra check */
	if (whereTo != NULL) {
		f = fileno(whereTo);
	} 
	if (f < 0) {
		PrintDiagnostic(DIAGERROR, "DirectDiagnostic: fileno failed", NULL);
		return;
	}
#endif
	GetNWSLock(&lock);
	diagDestinations[level] = whereTo;
	ReleaseNWSLock(&lock);
}


FILE *
DiagnosticsDirection(DiagLevels level) {
	return diagDestinations[level];
}


void
PositionedDiagnostic(DiagLevels level,
                     const char *fileName, 
                     int line,
                     const char *message,
                     ...) {

	va_list arguments;
	char *extendedMessage;

	/* we assume that NWS lines won't be past 10 digits */
	extendedMessage = (char *)MALLOC(strnlen(fileName, MAX_FILENAME_LENGTH)
				+ strnlen(message, MAX_MESSAGE_LENGTH) + 11);
	if (extendedMessage == NULL) {
		/* out of memory */
		PrintDiagnostic(DIAGERROR, "PositionedDiagnostic: out of memory", NULL);
		return;
	}

	va_start(arguments, message);
	sprintf(extendedMessage, "%s:%d %s", fileName, line, message);
	PrintDiagnostic(level, extendedMessage, arguments);
	va_end(arguments);

	free(extendedMessage);
}


void
Diagnostic(DiagLevels level,
           const char *message,
           ...) {
	va_list arguments;

	va_start(arguments, message);
	PrintDiagnostic(level, message, arguments);
	va_end(arguments);
}
