/* $Id$ */

#ifndef TIMEOUTS_H
#define TIMEOUTS_H

/*
 * This modules defines functions used to keep track of adaptive
 * timeouts. You can use these functions to (for example) keep track of
 * how long you should wait to receive a packet from a remote host.
 */

#include "dnsutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * We reserve timeouts for receive and send packets, connection time and
 * we leave space for 3 user definable timeouts.
 */
typedef enum {RECV, SEND, CONN, USER1, USER2, USER3} timeoutTypes;
#define TIMEOUT_TYPES_NUMBER 6

/**
 * Sets the bounds on a specific class of timeouts. 
 */
void
SetDefaultTimeout(timeoutTypes type, double min, double max);

/**
 * get the current defaults timeouts
 */
void
GetDefaultTimeout(timeoutTypes type, double *min, double *max);


/**
 * Returns the current timeouts value for the specific class given the
 * IPAddress and the #size# of the data to be sent/received. If #size# is
 * <= 0 it will be ignored.
 */
double
GetTimeOut(timeoutTypes type, IPAddress addr, long size);

/**
 * Reports how long it took to perform the action (say send or receive a
 * packet) last time and if it timed out and how many bytes were
 * sent/received (if applicable).
 */
void
SetTimeOut(timeoutTypes type, IPAddress addr, double duration, long size,  int timedOut);


#ifdef __cplusplus
}
#endif

#endif /* TIMEOUTS_H */
