#ifndef __ERROR_H
#define __ERROR_H


#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#define EREAD				((int)100)	/* a read pipe operation failed												*/
#define EREADPIPE			((int)101)	/* the pipe used to read from the stdout of the command is broken			*/
#define ETIMEOUT			((int)102)	/* the command is timeouted													*/
#define EWRITE				((int)103)	/* a write operation failed													*/
#define EWRITEPIPE			((int)104)	/* the pipe used to write to the stdin of the command is broken				*/
#define EEXEC				((int)105)	/* can't execute the command												*/
#define EWAIT				((int)106)	/* the wait function failed													*/
#define ECMDNOTFOUND		((int)107)	/* the command is not found													*/
#define EEXITCODENOTMATCH	((int)108)	/* the exit codes don't match												*/
#define EOUTPUTNOTMATCH		((int)109)	/* the outputs don't match													*/
#define ESIGNOTMATCH		((int)110)	/* the signals don't match													*/
#define EUNEXPECTEDSIG		((int)111)	/* unexpected signal caught													*/
#define ESIGNOTRECEIPT		((int)112)	/* the expected signal is not receipt										*/
#define EFILENOTFOUND		((int)113)	/* the specified tesh file is not found										*/
#define EGETCWD				((int)114)	/* this is a system error : the getcwd() function failed (impossible)		*/
#define EDIRNOTFOUND		((int)115)	/* the specified directory is not found										*/
#define ECHDIR				((int)116)	/* this is a system error : the chdir() function failed (impossible)		*/
#define EPROCCMDLINE		((int)117)	/* this is an internal error : the process_command_line() function failed	*/
#define ENOARG				((int)118)	/* a none optional argument is not specified in the command line			*/
#define ENOTPOSITIVENUM		((int)119)	/* the argument of the option is not strictly positive						*/
#define ESYNTAX				((int)120)	/* syntax error																*/
#define EINVALIDTIMEOUT		((int)121)	/* the timeout value specified by the metacommand is invalid				*/
#define EINVALIDEXITCODE	((int)122)	/* the expected exit code value specified by the metacommand is invalid		*/
#define ESIGNOTSUPP			((int)123)	/* the signal specified by the metacommand is not supported					*/
#define ELEADTIME			((int)124)	/* global timeout															*/
#define EREADMENOTFOUND		((int)125)	/* unable to locate the README.txt file										*/
#define EINCLUDENOTFOUND	((int)126)	/* the include file specified by a metacommand is not found					*/
#define ESUFFIXTOOLONG		((int)127)	/* the suffix is too long													*/
#define EFILENOTINSPECDIR	((int)128) /* file not found in the specified directories								*/
#define EFILENOTINCURDIR	((int)129) /* file not found in the current directory									*/


const char*
error_get_at(int pos, int* code);

const char*
error_to_string(int errcode);

#ifdef __cplusplus
}
#endif


#endif /* !__ERROR_H */

