#ifndef __XERRNO_H
#define __XERRNO_H

#include <com.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECMDTIMEDOUT		((int)102)	/* Command timed out														*/
#define EEXEC				((int)105)	/* can't execute the command												*/
#define EWAIT				((int)106)	/* the wait function failed													*/
#define ECMDNOTFOUND		((int)107)	/* the command is not found													*/
#define EEXITCODENOTMATCH	((int)108)	/* the exit codes don't match												*/
#define EOUTPUTNOTMATCH		((int)109)	/* the outputs don't match													*/
#define ESIGNOTMATCH		((int)110)	/* the signals don't match													*/
#define EUNXPSIG			((int)111)	/* Unexpected signal caught													*/
#define ESIGNOTRECEIPT		((int)112)	/* the expected signal is not receipt										*/
#define EFILENOTFOUND		((int)113)	/* the specified tesh file is not found										*/
#define EGETCWD				((int)114)	/* this is a system error : the getcwd() function failed (impossible)		*/
#define EDIRNOTFOUND		((int)115)	/* the specified directory is not found										*/
#define EPROCCMDLINE		((int)116)	/* this is an internal error : the process_command_line() function failed	*/
#define ENOARG				((int)117)	/* a none optional argument is not specified in the command line			*/
#define ENOTPOSITIVENUM		((int)118)	/* the argument of the option is not strictly positive						*/
#define ESYNTAX				((int)119)	/* syntax error																*/
#define EINVALIDTIMEOUT		((int)120)	/* the timeout value specified by the metacommand is invalid				*/
#define EINVALIDEXITCODE	((int)121)	/* the expected exit code value specified by the metacommand is invalid		*/
#define ESIGNOTSUPP			((int)122)	/* the signal specified by the metacommand is not supported					*/
#define ELEADTIME			((int)123)	/* global timeout															*/
#define EREADMENOTFOUND		((int)124)	/* unable to locate the README.txt file										*/
#define EINCLUDENOTFOUND	((int)125)	/* the include file specified by a metacommand is not found					*/
#define ESUFFIXTOOLONG		((int)126)	/* the suffix is too long													*/
#define EFILENOTINSPECDIR	((int)127) /* file not found in the specified directories								*/
#define EFILENOTINCURDIR	((int)128) /* file not found in the current directory									*/
#define EINVCMDLINE			((int)139) /* invalid command line														*/

#ifndef EALREADY
#define EALREADY			((int)131)
#endif

const char*
error_get_at(int pos, int* code);

const char*
error_to_string(int errcode);

void
error_register(const char* reason, int errcode, const char* command, const char* unit);

#ifdef __cplusplus
}
#endif


#endif /* !__XERRNO_H */

