/* $Id$ */


#ifndef PROTOCOL_H
#define PROTOCOL_H


/*
 * This module defines functions useful in establishing and maintaining
 * connections to other processes on local or remote hosts.  The name is an
 * anachronism.
 */


#include <sys/types.h>    /* pid_t */
#include "dnsutil.h"      /* IPAddress */

#ifdef __cplusplus
extern "C" {
#endif

/* A definition for socket call-back functions (see NotifyOnDisconnection()). */
typedef void (*SocketFunction)(Socket);


/*
 * Attempts to establish a connection to the server listening to #addr#:#port#.
 * If successful within #timeOut# seconds, returns 1 and sets #sock# to the
 * socket bound to the connection, else returns 0. A #timeOut# of 0 will
 * disable timeouts while a negative value will use adaptive timeouts.
 */
int
CallAddr(IPAddress addr, 
         short port, 
         Socket *sock,
         double timeOut);


/*
 * Closes connections opened via calls to this module's functions.  Each
 * parameter is a boolean value indicated whether that type of connection
 * should be included in those closed.
 */
void
CloseConnections(int closeEars,
                 int closePipes,
                 int closeSockets);
#define CloseAllConnections() CloseConnections(1, 1, 1)


/*
 * Tests all connections opened via calls to this module's functions and
 * closes any that are no longer connected.  Returns the number of connections
 * closed.
 */
int
CloseDisconnections(void);


/**
 * Tears down #sock#.  If #waitForPeer# is non-zero, the function waits this
 * many seconds for the host on the other end to close the connection and fails
 * if no such close is detected.  If this parameter is zero, the function
 * closes #sock# immediately.  Returns 1 and sets #sock# to NO_SOCKET if
 * successful, else returns 0.
 */
int
CloseSocket(Socket *sock,
            int waitForPeer);
#define DROP_SOCKET(sock) CloseSocket(sock, 0)


/**
 * Removed all records of #sock# from our FD_SETs
 */
void
ClearSocket(Socket sock);

/*
** Spawns a new process, a duplicate of the running one.  Returns 1 if
** successful, 0 otherwise.  Returns in #pid# a 0 to the child process and the
** process id of the child to the parent.  Both processes are given a pair of
** connections in the Socket parameters that can be used to communicate between
** the parent and child.  The parent process should send information to the
** child via #parentToChild# and receive information via #childToParent#; the
** child reads from the former and writes to the latter.  The parameters may be
** NULL indicating that communication is unidirectional (one parameter NULL),
** or that no communication will take place (both NULL).
*/
int
CreateLocalChild(pid_t *pid,
                 Socket *parentToChild,
                 Socket *childToParent);


/*
** Attempts to bind to any port between #startingPort# and #endingPort#,
** inclusive.  If successful, returns 1 and sets #ear# to the bound socket and
** #earPort# to the port to which it is bound, else returns 0.
*/
int
EstablishAnEar(unsigned short startingPort,
               unsigned short endingPort,
               Socket *ear,
               unsigned short *earPort);


/**
 * Checks all connections established via calls to this module for
 * incoming messages.  If one is detected within #timeOut# seconds,
 * returns 1, sets #sd# to the socket containing the message, and sets
 * #ldap# to the type of message (0 if NWS, 1 if LDAP).  If no message
 * detected, returns 0. 
 * 
 * NOTE: you have to use SocketIsAvailable to notify IncomingRequest that
 * the socket is again available (once the socket is been returned from
 * IncomingRequest) */
int 
IncomingRequest(double timeOut,
                Socket *sd,
                int *ldap);


/**
 * When a socket is returned by IncomingRequest, that socket won't be
 * listen till this function is called.
 * Return 0 upon failure.
 */
int
SocketIsAvailable(Socket sd);

/**
 * Tell NWS that a specific socket is still in use and IncomingRequest
 * shouldn't listen to it.
 */
int
SocketInUse(Socket sd);

/*
** Returns 1 or 0 depending on whether or not #sd# is connected to another
** process.
*/
int
IsOkay(Socket sd);

/**
 * returns 1 or 0 depending on wheter the socket is a pipe or not 
 */
int
IsPipe(Socket sd);


/*
** Registers a function that should be called whenever a socket is disconnected,
** either directly via a call to CloseSocket(), or indirectly because the peer
** termintes the connection.  The function is passed the closed socket after it
** has been closed.
*/
void
NotifyOnDisconnection(SocketFunction notifyFn);


/*
** Pass socket #sock# along to child #child# -- call after a successful call
** to CreateLocalChild().  The parent process will no longer listen for
** activity on #sock#.  Closing the socket will be put off until after the
** child dies, just in case the parent and child share descriptors.
*/
int
PassSocket(Socket *sock,
           pid_t child);


/**
 * Receives #byteSize# bytes from connection #sd# and stores them in the
 * memory referenced by #bytes#.  The caller must assure that #bytes# is at
 * least #byteSize# bytes long.  Returns 1 if successful within #timeOut#
 * seconds, else 0.  If #timeOut# is zero, timeout are disabled alltogether.
 */
int
RecvBytes(Socket sd,
          void *byte,
          size_t byteSize,
          double timeOut);

/*
 * Sends #bytesSize# bytes from the memory pointed to by #bytes# on connection
 * #sd#.  Returns 1 if successful within #timeOut# seconds, else 0.
 * If #timeOut# is zero, timeout are disabled alltogether..
 */
int
SendBytes(Socket sd,
          const void *bytes,
          size_t byteSize,
          double timeOut);


/*
** A signal handler for dealing with signals (specifically SIGPIPE) that
** indicate a bad socket.
*/
void
SocketFailure(int sig);


#ifdef __cplusplus
}
#endif

#endif
