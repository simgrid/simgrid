#ifndef __XSIGNAL_H
#define __XSIGNAL_H

#include <signal.h>
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
#ifdef _XBT_WIN32
  
/* terminal line hangup							*/ 
#ifndef SIGHUP
#define SIGHUP		1
#endif  /*  */
  
/* interrupt program							*/ 
#ifndef	SIGINT
#define SIGINT		2
#endif  /*  */
  
/* quit program									*/ 
#ifndef SIGQUIT
#define SIGQUIT		3
#endif  /*  */
  
/* illegal instruction							*/ 
#ifndef	SIGILL
#define SIGILL		4
#endif  /*  */
  
/* trace trap									*/ 
#ifndef	SIGTRAP
#define SIGTRAP		5
#endif  /*  */
  
/* abnormal termination triggered by abort call	*/ 
#ifndef	SIGABRT
#define SIGABRT		6
#endif  /*  */
  
/* floating point exception						*/ 
#ifndef	SIGFPE
#define SIGFPE		8
#endif  /*  */
  
/* kill program									*/ 
#ifndef	SIGKILL
#define SIGKILL		9
#endif  /*  */
  
/* bus error									*/ 
#ifndef SIGBUS
#define SIGBUS		10
#endif  /*  */
  
/* segment violation							*/ 
#ifndef	SIGSEGV
#define SIGSEGV		11
#endif  /*  */
  
/* non-existent system call invoked				*/ 
#ifndef SIGSYS
#define SIGSYS		12
#endif  /*  */
  
/* write on a pipe with no reader				*/ 
#ifndef SIGPIPE
#define SIGPIPE		13
#endif  /*  */
  
/* real-time timer expired						*/ 
#ifndef SIGALRM
#define SIGALRM		14
#endif  /*  */
  
/* software termination signal from kill			*/ 
#ifdef	SIGTERM
#define SIGTERM		15
#endif  /*  */
  
/* urgent condition present on socket				*/ 
#ifndef SIGURG
#define SIGURG		16
#endif  /*  */
  
/* stop (cannot be caught orignored)				*/ 
#ifndef SIGSTOP
#define SIGSTOP		17
#endif  /*  */
  
/* stop signal generated from keyboard				*/ 
#ifndef SIGTSTP
#define SIGTSTP		18
#endif  /*  */
  
/* continue after stop								*/ 
#ifndef SIGCONT
#define SIGCONT		19
#endif  /*  */
  
/* child status has changed							*/ 
#ifndef SIGCHLD
#define SIGCHLD		20
#endif  /*  */
  
/* background read attempted from control terminal	*/ 
#ifndef SIGTTIN
#define SIGTTIN		21
#endif  /*  */
  
/* background write attempted to control terminal	*/ 
#ifndef SIGTTOU
#define SIGTTOU		22
#endif  /*  */
  
/* I/O is possible on a descriptor see fcntl(2))	*/ 
#ifndef SIGIO
#define SIGIO		23
#endif  /*  */
  
/* cpu time limit exceeded (see setrlimit(2))		*/ 
#ifndef SIGXCPU
#define SIGXCPU		24
#endif  /*  */
  
/* file size limit exceeded (see setrlimit(2))		*/ 
#ifndef SIGXFSZ
#define SIGXFSZ		25
#endif  /*  */
  
/* virtual time alarm (see setitimer(2))			*/ 
#ifndef SIGVTALRM
#define SIGVTALRM	26
#endif  /*  */
  
/* profiling timer alarm (see setitimer(2))			*/ 
#ifndef SIGPROF
#define SIGPROF		27
#endif  /*  */
  
/*  window size change								*/ 
#ifndef SIGWINCH
#define SIGWINCH	28
#endif  /*  */
  
/* user defined signal 1							*/ 
#ifndef SIGUSR1
#define SIGUSR1		30
#endif  /*  */
  
/* user defined signal 2							*/ 
#ifndef SIGUSR2
#define SIGUSR2		31
#endif  /*  */
    int  is_an_unhandled_exception(DWORD exit_code);
   
/*  
 *return a non-zero value if status was returned for a child process that terminated normally. 
 */ 
#define WIFEXITED(__status)		!is_an_unhandled_exception((__status))
  
/* if the value of WIFEXITED(__status) is non-zero, this macro evaluates the value the child 
 * process returned from main().
 */ 
#define WEXITSTATUS(__status)	(__status)
  
/* return a non-zero value if status was returned for a child process that terminated due to the 
 * receipt of a signal that was not caught 
 */ 
#define WIFSIGNALED(__status)	is_an_unhandled_exception((__status))
  
/* if the value of WIFSIGNALED(__status) is non-zero, this macro evaluates to the number of the 
 * signal that caused the termination of the child process.
 */ 
#define WTERMSIG(__status)		(__status)
  
#endif  /* _XBT_WIN32 */
   
#ifdef _XBT_WIN32
  const char * signal_name(DWORD got, const char *expected);
  
#else   /*  */
  const char * signal_name(unsigned int got, char *expected);
  
#endif  /*  */
  int  sig_exists(const char *sig_name);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__XSIGNAL_H */
