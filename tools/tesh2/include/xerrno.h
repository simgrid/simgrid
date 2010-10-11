#ifndef __XERRNO_H
#define __XERRNO_H

#include <com.h>
#include <errno.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
#define ECMDTIMEDOUT		((int)102)	/* Command timed out														*/
#define ECMDNOTFOUND		((int)107)	/* the command is not found													*/
#define EEXITCODENOTMATCH	((int)108)	/* the exit codes don't match												*/
#define EOUTPUTNOTMATCH		((int)109)	/* the outputs don't match													*/
#define ESIGNOTMATCH		((int)110)	/* the signals don't match													*/
#define EUNXPSIG			((int)111)	/* Unexpected signal caught													*/
#define ESIGNOTRECEIVED		((int)112)	/* the expected signal is not receipt										*/
#define EPROCCMDLINE		((int)116)	/* this is an internal error : the process_command_line() function failed	*/
#define ENOARG				((int)117)	/* a none optional argument is not specified in the command line			*/
#define ENOTPOSITIVENUM		((int)118)	/* the argument of the option is not strictly positive						*/
#define ESYNTAX				((int)119)	/* syntax error																*/
#define ELEADTIME			((int)123)	/* global timeout															*/
#define EREADMENOTFOUND		((int)124)	/* unable to locate the README.txt file										*/
#define EINCLUDENOTFOUND	((int)125)	/* the include file specified by a metacommand is not found					*/
#define ESUFFIXTOOLONG		((int)126)	/* the suffix is too long													*/
#define EINVCMDLINE			((int)139) /* invalid command line														*/
  
#ifndef EALREADY
#define EALREADY			((int)131)
#endif  /*  */
   const char * error_get_at(int pos, int *code);
  const char * error_to_string(int errcode, int kind);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__XERRNO_H */

