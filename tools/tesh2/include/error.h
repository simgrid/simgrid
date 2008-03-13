#ifndef __ERROR_H
#define __ERROR_H


#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#define EREAD				((int)2000)	/* a read pipe operation failed												*/
#define EREADPIPE			((int)2001)	/* the pipe used to read from the stdout of the command is broken			*/
#define ETIMEOUT			((int)2002)	/* the command is timeouted													*/
#define EWRITE				((int)2003)	/* a write operation failed													*/
#define EWRITEPIPE			((int)2004)	/* the pipe used to write to the stdin of the command is broken				*/
#define EEXEC				((int)2005)	/* can't execute the command												*/
#define EWAIT				((int)2006)	/* the wait function failed													*/
#define ECMDNOTFOUND		((int)2007)	/* the command is not found													*/
#define EEXITCODENOTMATCH	((int)2008)	/* the exit codes don't match												*/
#define EOUTPUTNOTMATCH		((int)2009)	/* the outputs don't match													*/
#define ESIGNOTMATCH		((int)2010)	/* the signals don't match													*/
#define EUNEXPECTEDSIG		((int)2011)	/* unexpected signal caught													*/
#define ESIGNOTRECEIPT		((int)2012)	/* the expected signal is not receipt										*/
#define EFILENOTFOUND		((int)2013)	/* the specified tesh file is not found										*/
#define EGETCWD				((int)2014)	/* this is a system error : the getcwd() function failed (impossible)		*/
#define EDIRNOTFOUND		((int)2015)	/* the specified directory is not found										*/
#define ECHDIR				((int)2016)	/* this is a system error : the chdir() function failed (impossible)		*/
#define EPROCESSCMDLINE		((int)2017)	/* this is an internal error : the process_command_line() function failed	*/
#define EARGNOTSPEC			((int)2018)	/* a none optional argument is not specified in the command line			*/
#define ENOTPOSITIVENUM		((int)2019)	/* the argument of the option is not strictly positive						*/
#define ESYNTAX				((int)2020)	/* syntax error																*/
#define EINVALIDTIMEOUT		((int)2021)	/* the timeout value specified by the metacommand is invalid				*/
#define EINVALIDEXITCODE	((int)2022)	/* the expected exit code value specified by the metacommand is invalid		*/
#define ESIGNOTSUPP			((int)2023)	/* the signal specified by the metacommand is not supported					*/
#define ELEADTIME			((int)2024)	/* global timeout															*/
#define EREADMENOTFOUND		((int)2025)	/* unable to locate the README.txt file										*/
#define EINCLUDENOTFOUND	((int)2026)	/* the include file specified by a metacommand is not found					*/
#define ESUFFIXTOOLONG		((int)2027)	/* the suffix is too long													*/
#define EFILENOTINSPECDIR	((int)2028) /* file not found in the specified directories								*/
#define EFILENOTINCURDIR	((int)2029) /* file not found in the current directory									*/

const char*
error_to_string(int error);

#ifdef __cplusplus
}
#endif


#endif /* !__ERROR_H */

