/* $Id$ */


#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H


/*
 * This module manages the production of diagnostic messages.
 */


#include <stdio.h> /* FILE */


#ifdef __cplusplus
extern "C" {
#endif


/*
** Different types of diagnostics.
*/
typedef enum {DIAGLOG, DIAGINFO, DIAGWARN, DIAGERROR, DIAGFATAL, DIAGDEBUG} DiagLevels;


#define DIAGSUPPRESS 0

/* 
 * Maximum size for the message and for filenames
 */
#define MAX_MESSAGE_LENGTH 512
#define MAX_FILENAME_LENGTH 512

/*
** Directs #level#-level diagnostic messages to the file #whereTo#.  Messages
** for any level may be supressed by passing DIAGSUPPRESS as the second
** parameter to this routine.  By default, all messages are suppressed.
*/
void
DirectDiagnostics(DiagLevels level,
                  FILE *whereTo);


/*
** Returns the file to which #level#-level diagnostics are being directed.  May
** be used to save off the direction for a change/restore, or for writing out
** non-message-based information.
*/
FILE *
DiagnosticsDirection(DiagLevels level);


/*
** Produces the #level#-level diagnostic message #message#, which is used as
** the format string in a call to fprintf().  #sourcefilename# and
** #sourcelinenumber# are appended to the message.  Additional fprintf()
** arguments may be passed after #message#.
*/
void
PositionedDiagnostic(DiagLevels level,
                     const char *fileName, 
                     int line,
                     const char *message,
                     ...);


/*
** Produces the #level#-level diagnostic message #message#, which is used as
** the format string in a call to fprintf().  Additional fprintf()
** arguments may be passed after #message#.
*/
void
Diagnostic(DiagLevels level,
           const char *message,
           ...);


/*
** These macros are provided for the usual case where DIAGFATAL messages are
** emitted just before aborting the program and DIAGERROR messages just before
** returning a failure value from a function.  The ERROR, WARN, INFO, and LOG
** macros are just a handy shorthand.  Unfortunately, the varargs concept
** doesn't extend to macros, which means separate macros have to be defined for
** various parameter counts.  The peculiar use of do ... while here prevents
** compilation problems in cases such as:
** if (foo < bar) FAIL("ick!"); else something();
** The embedded if (1) satisfies the picky compilers that complain about
** execution not reaching the loop test otherwise.
*/
#define ABORT(message) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__, __LINE__,message); \
    if (1) exit(1); \
  } while (0)
#define ABORT1(message,a) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__,__LINE__,message,a); \
    if (1) exit(1); \
  } while (0)
#define ABORT2(message,a,b) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__,__LINE__,message,a,b); \
    if (1) exit(1); \
  } while (0)
#define ABORT3(message,a,b,c) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__,__LINE__,message,a,b,c); \
    if (1) exit(1); \
  } while (0)
#define ABORT4(message,a,b,c,d) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__,__LINE__,message,a,b,c,d); \
    if (1) exit(1); \
  } while (0)
#define ABORT5(message,a,b,c,d,e) \
  do { \
    PositionedDiagnostic(DIAGFATAL,__FILE__,__LINE__,message,a,b,c,d,e); \
    if (1) exit(1); \
  } while (0)

#define FAIL(message) \
  do { \
    PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message); \
    if (1) return(0); \
  } while (0)
#define FAIL1(message,a) \
  do { \
    PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a); \
    if (1) return(0); \
  } while (0)
#define FAIL2(message,a,b) \
  do { \
    PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b); \
    if (1) return(0); \
  } while (0)
#define FAIL3(message,a,b,c) \
  do { \
    PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c); \
    if (1) return(0); \
  } while (0)
#define FAIL4(message,a,b,c,d) \
  do { \
    PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c,d); \
    if (1) return(0); \
  } while (0)
#define FAIL5(message,a,b,c,d,e) \
  do {PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c,d,e); \
    if (1) return(0); \
  } while (0)

#define ERROR(message) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message)
#define ERROR1(message,a) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a)
#define ERROR2(message,a,b) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b)
#define ERROR3(message,a,b,c) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c)
#define ERROR4(message,a,b,c,d) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c,d)
#define ERROR5(message,a,b,c,d,e) \
  PositionedDiagnostic(DIAGERROR,__FILE__,__LINE__,message,a,b,c,d,e)

#define WARN(message) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message)
#define WARN1(message,a) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message,a)
#define WARN2(message,a,b) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message,a,b)
#define WARN3(message,a,b,c) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message,a,b,c)
#define WARN4(message,a,b,c,d) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message,a,b,c,d)
#define WARN5(message,a,b,c,d,e) \
  PositionedDiagnostic(DIAGWARN,__FILE__,__LINE__,message,a,b,c,d,e)

#define INFO(message) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message)
#define INFO1(message,a) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message,a)
#define INFO2(message,a,b) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message,a,b)
#define INFO3(message,a,b,c) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message,a,b,c)
#define INFO4(message,a,b,c,d) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message,a,b,c,d)
#define INFO5(message,a,b,c,d,e) \
  PositionedDiagnostic(DIAGINFO,__FILE__,__LINE__,message,a,b,c,d,e)

#define LOG(message) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message)
#define LOG1(message,a) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message,a)
#define LOG2(message,a,b) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message,a,b)
#define LOG3(message,a,b,c) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message,a,b,c)
#define LOG4(message,a,b,c,d) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message,a,b,c,d)
#define LOG5(message,a,b,c,d,e) \
  PositionedDiagnostic(DIAGLOG,__FILE__,__LINE__,message,a,b,c,d,e)

/* /usr/include/macros.h defines a DEBUG macro, so we use DDEBUG instead. */
#define DDEBUG(message) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message)
#define DDEBUG1(message,a) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message,a)
#define DDEBUG2(message,a,b) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message,a,b)
#define DDEBUG3(message,a,b,c) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message,a,b,c)
#define DDEBUG4(message,a,b,c,d) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message,a,b,c,d)
#define DDEBUG5(message,a,b,c,d,e) \
  PositionedDiagnostic(DIAGDEBUG,__FILE__,__LINE__,message,a,b,c,d,e)

#ifdef __cplusplus
}
#endif


#endif
