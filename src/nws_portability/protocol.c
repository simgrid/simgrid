/* $Id$ */

#include "config_portability.h"

#include <unistd.h>       /* close() pipe() read() write() */
#include <signal.h>       /* close() pipe() read() write() */
#include <netinet/in.h>   /* sometimes required for #include <arpa/inet.h> */
#include <netinet/tcp.h>  /* TCP_NODELAY */
#ifdef HAVE_INTTYPES_H
#	include <inttypes.h>
#endif
#include <arpa/inet.h>    /* inet_ntoa() */
#include <netdb.h>        /* getprotobyname() */
#include <sys/time.h>     /* struct timeval */
#include <errno.h>        /* errno */
#include <sys/wait.h>     /* waitpid() */
#include <sys/socket.h>   /* getpeername() socket() */
#include <stdlib.h>
#ifdef WITH_LDAP
#include <lber.h>
#endif

#include "diagnostic.h"
#include "osutil.h"
#include "protocol.h"
#include "strutil.h"
#include "dnsutil.h"
#include "timeouts.h"


static void *lock = NULL;		/* local mutex */

/* Global variables: they need to be protected with locks when accessed */
#define MAX_NOTIFIES 40
static SocketFunction disconnectFunctions[MAX_NOTIFIES];
static fd_set connectedEars;
static fd_set connectedPipes;
static fd_set connectedSockets;
static fd_set inUse;

/* IncomingRequest requires some care in case we have thread. I
 * don't want multiple IncomingRequest called at the same time:
 * if that happens you may need to rewrite your code. Just to be
 * sure this doesn't happen I use a cheat test-and-set relying
 * upon the global lock. It may be easier to use semaphores, but
 * given that we don't use them anywhere else ....
 */
#ifdef HAVE_PTHREAD_H
static short running = 0;
#endif

/* This is used when it's time to call CloseDisconnections(): when
 * receiving a SIGPIPE, this flag is set to 1 so that IncomingRequest,
 * SendBytes and RecvBytes (the functions which operates on sockets) will
 * call CloseDisconnections to pick up the disconnected socket */
static short needDisconnect = 0;

#ifdef WITH_THREAD
extern void LockMessageSystem();
extern void UnlockMessageSystem();
#else
#define LockMessageSystem()
#define UnlockMessageSystem()
#endif


/*
 * Beginning of connection functions.
 */


static int
TcpProtoNumber(void);


/*
 * Remove #sock# from all maintained socket sets.
 *
 * It should be thread safe.
 */
void
ClearSocket(Socket sock) {
	/* operates on global variables */
	GetNWSLock(&lock);
	FD_CLR(sock, &connectedPipes);
	FD_CLR(sock, &connectedSockets);
	FD_CLR(sock, &connectedEars);
	/* clear also the inUse state */
	FD_CLR(sock, &inUse);
	ReleaseNWSLock(&lock);
}


/* It should be thread safe */
int
ConditionSocket(Socket sd) {
	int one = 1;

	if(setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char *)&one, sizeof(int)) < 0) {
		WARN("ConditionSocket: keepalive option failed\n");
	}

	if(setsockopt(sd, TcpProtoNumber(), TCP_NODELAY, (char *)&one, sizeof(int)) < 0) {
		WARN("ConditionSocket: couldn't set NODELAY flag\n");
	}

	return (1);
}


/*
 * Time-out signal handler for CallAddr().
 */
void
ConnectTimeOut(int sig) {
	WARN("Connect timed out\n");
}


/*
 * Notifies all registered functions that #sock# has been closed.
 *
 * We should lock the call ...
 */
static void
DoDisconnectNotification(Socket sock) {
	int i;

	for(i = 0; i < MAX_NOTIFIES; i++) {
		if(disconnectFunctions[i] != NULL) {
			disconnectFunctions[i](sock);
		}
	}
}


/*
 * Time-out signal handler for RecvBytes().
 */
void
RecvTimeOut(int sig) {
	WARN("Send/Receive timed out\n");
}


/*
 * Returns the tcp protocol number from the network protocol data base.
 * 
 * getprotobyname() is not thread safe. We need to lock it.
 */
static int
TcpProtoNumber(void) {
	struct protoent *fetchedEntry;
	static int returnValue = 0;

	if(returnValue == 0) {
		GetNWSLock(&lock);
		fetchedEntry = getprotobyname("tcp");
		if(fetchedEntry != NULL) {
			returnValue = fetchedEntry->p_proto;
		}
		ReleaseNWSLock(&lock);
	}

	return returnValue;
}


/* thread safe */
int
CallAddr(IPAddress addr,
         short port,
         Socket *sock,
         double timeOut) {

	struct sockaddr_in server; /* remote host address */
	Socket sd;
	double start;
	double ltimeout = 0;
	void (*was)(int);
	int tmp_errno, ret = 0;
	char *peer;

	memset((char *)&server, 0, sizeof(server));
	server.sin_addr.s_addr = addr;
	server.sin_family = AF_INET;
	server.sin_port = htons((u_short)port);

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if(sd < 0) {
		*sock = NO_SOCKET;
		ERROR("CallAddr: cannot create socket to server\n");
		return 0;
	}

	ConditionSocket(sd);

	/* set the adaptive timeout or the user selected one */
	if (timeOut >= 0) {
		ltimeout = timeOut;
	} else {
		/* adaptive timeouts */
		ltimeout = GetTimeOut(CONN, addr, 0);
	}
	if (ltimeout > 0) {
		DDEBUG1("CallAddr: setting timer to %.2f\n", ltimeout);
		if (SignalAlarm(ConnectTimeOut, &was) == 0) {
			WARN("Failed to set the alarm signal! exiting\n");
			return 0;
		}
		SetRealTimer((unsigned int)ltimeout);
	}

	/* let's time it */
	start = CurrentTime();
  
	if(connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		GetNWSLock(&lock);
		/* save a copy or errno */
		tmp_errno = errno;
		ReleaseNWSLock(&lock);

		shutdown(sd, 2);
		close(sd);

		/* get how long it took to get it wrong */
		start = CurrentTime() - start;

		if(tmp_errno == EINTR) {
			WARN("CallAddr: connect timed out\n");
		} else {
			ERROR1("CallAddr: connect failed (errno=%d)\n", tmp_errno);
		}
		*sock = NO_SOCKET;
	} else {
		/* get how long it took */
		start = CurrentTime() - start;

		*sock = sd;

		/* print log message */
		peer = IPAddressMachine_r(addr);
		LOG4("CallAddr: connected socket %d to %s:%d in %.2f seconds\n", 
		     sd, peer, port, start);
		FREE(peer);

		GetNWSLock(&lock);
		FD_SET(sd, &connectedSockets);
		ReleaseNWSLock(&lock);
	
		/* everything is cool */
		ret = 1;
	}

	if (timeOut != 0) {
		RESETREALTIMER;
		SignalAlarm(was, NULL);
		if (timeOut < 0) {
			/* adaptive timeouts */
			SetTimeOut(CONN, addr, start, 0, (ret==0));
		}
	}

	return ret;
}


/* it should be thread safe (we lock up access to connected*) */
void
CloseConnections(int closeEars,
                 int closePipes,
                 int closeSockets) {
	Socket dead;
	int i, tmp;

	if(closeEars) {
		for(i = 0; i < FD_SETSIZE; i++) {
			GetNWSLock(&lock);
			tmp = FD_ISSET(i, &connectedEars);
			ReleaseNWSLock(&lock);
			if(tmp) {
				dead = i;
				DROP_SOCKET(&dead);
			}
		}
	}
	if(closePipes) {
		for(i = 0; i < FD_SETSIZE; i++) {
			GetNWSLock(&lock);
			tmp = FD_ISSET(i, &connectedPipes);
			ReleaseNWSLock(&lock);
			if(tmp) {
				dead = i;
				DROP_SOCKET(&dead);
			}
		}
	}
	if(closeSockets) {
		for(i = 0; i < FD_SETSIZE; i++) {
			GetNWSLock(&lock);
			tmp = FD_ISSET(i, &connectedSockets);
			ReleaseNWSLock(&lock);
			if(tmp) {
				dead = i;
				DROP_SOCKET(&dead);
			}
		}
	}
}


/*
 * Returns 1 or 0 depending on whether or not #sd# is a connected socket.
 *
 * it should be thread safe.
 */
static int
IsConnected(Socket sd) {
	struct sockaddr peer_name_buff;

	SOCKLEN_T peer_name_buff_size = sizeof(peer_name_buff);
	return(getpeername(sd, &peer_name_buff, &peer_name_buff_size) >= 0);
}


/* thread safe */
int
CloseDisconnections(void) {
	Socket dead, i;
	int returnValue = 0, tmp;

	for(i = 0; i < FD_SETSIZE; i++) {
		GetNWSLock(&lock);
		tmp = FD_ISSET(i, &connectedSockets);
		ReleaseNWSLock(&lock);
		if(tmp && !IsConnected(i)) {
			dead = i;
			DROP_SOCKET(&dead);
			returnValue++;
		}
	}

	return(returnValue);
}


/* (cross finger): thread safe */
int
CloseSocket(Socket *sock,
            int waitForPeer) {

	fd_set readFDs;
	Socket sd = *sock;
	struct timeval timeout;
	int tmp_errno;

	DDEBUG1("CloseSocket: Closing connection %d\n", *sock);

	if(*sock == NO_SOCKET) {
		return 1;  /* Already closed; nothing to do. */
	}

	if(waitForPeer > 0) {
		FD_ZERO(&readFDs);
		FD_SET(sd, &readFDs);
		timeout.tv_sec = waitForPeer;
		timeout.tv_usec = 0;

		if(select(FD_SETSIZE, &readFDs, NULL, NULL, &timeout) < 0) {
			ERROR2("CloseSocket: no response on select %d %d\n", sd, errno);
			return 0;
		}
	}

	GetNWSLock(&lock);
	tmp_errno = FD_ISSET(sd, &connectedPipes);
	ReleaseNWSLock(&lock);
	if(!tmp_errno) {
		if(shutdown(sd, 2) < 0) {
			GetNWSLock(&lock);
			tmp_errno = errno;
			ReleaseNWSLock(&lock);

			/* The other side may have beaten us to the reset. */
			if ((tmp_errno!=ENOTCONN) && (tmp_errno!=ECONNRESET)) {
				WARN1("CloseSocket: shutdown error %d\n", tmp_errno);
			}
		}
	}

	if(close(sd) < 0) {
		GetNWSLock(&lock);
		tmp_errno = errno;
		ReleaseNWSLock(&lock);

		WARN2("CloseSocket: close error %d (%s)\n", tmp_errno, strerror(tmp_errno));
	}

	ClearSocket(sd);
	DoDisconnectNotification(sd);
	*sock = NO_SOCKET;

	return(1);
}


#define READ_END 0
#define WRITE_END 1

int
CreateLocalChild(pid_t *pid,
                 Socket *parentToChild,
                 Socket *childToParent) {

  int childWrite[2];
  int parentWrite[2];
  int myEnd;

  if(parentToChild != NULL) {
    if(pipe(parentWrite) == -1) {
      FAIL1("CreateLocalChild: couldn't get pipe, errno: %d\n", errno);
    }
  }
  if(childToParent != NULL) {
    if(pipe(childWrite) == -1) {
      if(parentToChild != NULL) {
        close(parentWrite[0]);
        close(parentWrite[1]);
      }
      FAIL1("CreateLocalChild: couldn't get pipe, errno: %d\n", errno);
    }
  }

  *pid = fork();

  if(*pid == -1) {
    if(parentToChild != NULL) {
      close(parentWrite[0]);
      close(parentWrite[1]);
    }
    if(childToParent != NULL) {
      close(childWrite[0]);
      close(childWrite[1]);
    }
    FAIL2("CreateLocalChild: couldn't fork, errno: %d (%s)\n",
          errno, strerror(errno));
  }

  /* Close descriptors that this process won't be using. */
  if(parentToChild != NULL) {
    myEnd = (*pid == 0) ? READ_END : WRITE_END;
    close(parentWrite[1 - myEnd]);
    FD_SET(parentWrite[myEnd], &connectedPipes);
    *parentToChild = parentWrite[myEnd];
  }

  if(childToParent != NULL) {
    myEnd = (*pid == 0) ? WRITE_END : READ_END;
    close(childWrite[1 - myEnd]);
    FD_SET(childWrite[myEnd], &connectedPipes);
    *childToParent = childWrite[myEnd];
  }

  return(1);

}


/* it should be thread safe (provided that setsockopt, bind and listen
 * are thread safe) */
int
EstablishAnEar(unsigned short startingPort,
               unsigned short endingPort,
               Socket *ear,
               unsigned short *earPort) {

	int k32 = 32 * 1024;
	int on = 1;
	unsigned short port;
	Socket sd = NO_SOCKET;
	struct sockaddr_in server;

	for(port = startingPort; port <= endingPort; port++) {
		server.sin_port = htons((u_short)port);
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_family = AF_INET;
		if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			ERROR("EstablishAnEar: socket allocation failed\n");
			return 0;
		}
		(void)setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		/* Set the socket buffer sizes to 32k, which just happens
		 * to correspond to the most common option value for
		 * tcpMessageMonitor activities.  This allows us to use a
		 * client connection to conduct the experiment, rather
		 * than needing to configure and open a new connection.
		 * */
		(void)setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&k32, sizeof(k32));
		(void)setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&k32, sizeof(k32));
		if(bind(sd, (struct sockaddr *)&server, sizeof(server)) != -1 &&
					listen(sd, 5) != -1) {
			break;
		}
		close(sd);
	}

	if(port > endingPort) {
		FAIL2("EstablishAnEar: couldn't find a port between %d and %d\n", startingPort, endingPort);
	}

	GetNWSLock(&lock);
	FD_SET(sd, &connectedEars);
        DDEBUG1("Openned an ear on sock %d\n",sd);
	ReleaseNWSLock(&lock);

	*ear = sd;
	*earPort = port;

	DDEBUG1("EstablistAnEar: connected socket %d\n", sd);

	return(1);
}


/* thread safe ... hopefully. We #ifdef HAVE_THREAD_H for performance
 * reasons when threads are not required. */
int
IncomingRequest(double timeOut,
                Socket *sd,
                int *ldap) {

	Socket dead;
	Socket i;
	char lookahead;
	Socket newSock;
	double now;
	struct sockaddr_in peer_in;
	SOCKLEN_T peer_in_len = sizeof(peer_in);
	fd_set readFds;
	struct timeval tout;
	double wakeup;
	int tmp_errno, done = -1;

	/* nextToService is used to make sure that every connection gets
	 * a chance to be serviced.  If we always started checking at 0,
	 * very active connections with small descriptor numbers could
	 * starve larger-descriptor connections.  */
	/* static Socket nextToService = 0; */
	/* Obi: I don't use the "static" approach since may be trouble
	 * with thread. Instead of locking I use the CurrentTime() to
	 * (hopefully) start from a different place each time */
	Socket nextToService = ((int)CurrentTime() % FD_SETSIZE);

	/* let's check we are the only one running here ... */
#ifdef HAVE_PTHREAD_H
	GetNWSLock(&lock);
	if (running != 0) {
		ReleaseNWSLock(&lock);
		ERROR("IncomingRequest: another instance is running!\n");
		return 0;
	}
	running = 1;
	ReleaseNWSLock(&lock);
#endif

	*sd = NO_SOCKET;
	tout.tv_usec = 0;
	wakeup = CurrentTime() + timeOut;

	while (done == -1) {

		/* let's see if we need to check disconnected socket */
		if (needDisconnect == 1) {
			needDisconnect = 0;
			(void)CloseDisconnections();
		}

		/* is the timeout expired? */
		now = CurrentTime();
		if (now == -1 || now >= wakeup) {
			if (now == -1) {
				WARN("IncomingRequest: time() failed.\n");
			}
			done = 0;	/* didn't find anything */
			break;		/* let's get out of here */
		}

		/* Construct in readFds the union of connected ears,
		 * pipes, and sockets. */
		/* connected* are global variables and even though we
		 * only read them we play safe and we lock */
		FD_ZERO(&readFds);
		GetNWSLock(&lock);
		for(i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &connectedSockets) && !FD_ISSET(i, &inUse)) {
				FD_SET(i, &readFds);
			}
			if(FD_ISSET(i, &connectedPipes)) {
				FD_SET(i, &readFds);
			}
			if(FD_ISSET(i, &connectedEars)) {
				FD_SET(i, &readFds);
			}
		}
		ReleaseNWSLock(&lock);

		/* set the timeout */
		tout.tv_sec = (unsigned int)wakeup - (unsigned int)now;
		tout.tv_usec = 0;

		tmp_errno = select(FD_SETSIZE, &readFds, NULL, NULL, &tout);
		if (tmp_errno == -1) {
			/* save the errno value */
			GetNWSLock(&lock);
			tmp_errno = errno;
			ReleaseNWSLock(&lock);

			/* EINTR we have to go ahead and retry: nothing
			 * to do here */
			if(tmp_errno == EINTR) {
				continue;
			} else if(tmp_errno == EINVAL) {
				/* we blew it somehow -- (osf likely) */
				/* can be because (from man page):
				[EINVAL] The time limit specified by the
				timeout parameter is invalid.  The nfds
				parameter is less than 0, or greater than
				or equal to FD_SETSIZE.  One of the
				specified file descriptors refers to a
				STREAM or multiplexer that is linked
				(directly or indirectly) downstream from
				a multiplexer.  */

				ERROR4("IncomingRequest: invalid select - nfds: %d, rfds: %d, timeout: %d.%d", FD_SETSIZE, readFds, tout.tv_sec, tout.tv_usec);
			} else {
				ERROR1("IncomingRequest: select error %d\n", tmp_errno);
			}
			done = 0;	/* didn't find anything */
			break;			 /* done here */
		} else if (tmp_errno == 0) {
			/* this was a timeout */
			continue;
		} 

		/* let's find out what socket is available and for what */

		/* we do an all around loop to check all sockets
		 * starting from nextToService until we have one
		 * to service */
		for (i = -1; i != nextToService; i=(i+1) % FD_SETSIZE) {
			if (i == -1) {
				/* first time around */
				i = nextToService;
			}

			if(!FD_ISSET(i, &readFds)) {
				/* nothing to do here */
				continue;
			}

			if(FD_ISSET(i, &connectedEars)) {
				/* Add the new connection
				 * to connectedSockets. */
				newSock = accept(i, (struct sockaddr *)&peer_in, &peer_in_len);
				if(newSock == -1) {
					SocketFailure(SIGPIPE);
				} else {
					char *peer;

					ConditionSocket(newSock);
					peer = PeerName_r(newSock);

					DDEBUG2("IncomingRequest: connected socket %d to %s\n", newSock, peer);
					FREE(peer);

					/* operating on a global variable */
					GetNWSLock(&lock);
					FD_SET(newSock, &connectedSockets);
					ReleaseNWSLock(&lock);
				}
			} else if(FD_ISSET(i, &connectedPipes)) {
				/* we found a good one */
				*sd = i;
				*ldap = 0;
				done = 1;
				break;
			} else {
				/* Existing socket connection. */
				if(recv(i, &lookahead, 1, MSG_PEEK) > 0) {
					*sd = i;
#ifdef WITH_LDAP
 					*ldap = ((int) lookahead == LBER_SEQUENCE);
#else
					*ldap = 0;
#endif
#ifdef HAVE_PTHREAD_H
					/* the socket is in use:
					 * client needs to call
					 * SocketIsAvailable to
					 * free it */
					/* it only makes sense in
					 * a threaded environment */
					GetNWSLock(&lock);
					FD_SET(i, &inUse);
					ReleaseNWSLock(&lock);
#endif
					done = 1;
					break;
				} else {
					/* This is how we find
					 * out about connections
					 * closed by a peer.
					 * Drop it from our list
					 * of known connections.
					 */
					DDEBUG1("IncomingRequest: Dropping closed connection %d\n", i);

					dead = i;
					DROP_SOCKET(&dead);
				}
			}
		}
	}

	/* done */
#ifdef HAVE_PTHREAD_H
	GetNWSLock(&lock);
	running = 0;
	ReleaseNWSLock(&lock);
#endif

	return done;
}


/* thread safe */
int 
SocketIsAvailable(Socket sd) {
	int ret = 1;

	/* sanity check */
	if (sd < 0) {
		WARN("SocketIsAvailable: socket is negative\n");
		return 0;
	}

	/* check if the socket is in connectedSockets and is in use */
	GetNWSLock(&lock);
	FD_CLR(sd, &inUse);
	ReleaseNWSLock(&lock);

	return ret;
}

/* thread safe */
int
SocketInUse(Socket sd) {

	/* sanity check */
	if (sd < 0) {
		WARN("SocketInUse: socket is negative\n");
		return 0;
	}

	GetNWSLock(&lock);
	FD_SET(sd, &inUse);
	ReleaseNWSLock(&lock);

	return 1;
}

/* thread safe */
int
IsOkay(Socket sd) {
	fd_set readFds;
	fd_set writeFds;
	struct timeval timeout;

	if(sd < 0) {
		return 0;
	}

	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_SET(sd, &readFds);
	FD_SET(sd, &writeFds);
	timeout.tv_sec  = GetTimeOut(SEND, Peer(sd), 1);
	timeout.tv_usec = 0;

	return(select(FD_SETSIZE, NULL, &writeFds, NULL, &timeout) == 1);

}


/* thread safe */
void
NotifyOnDisconnection(SocketFunction notifyFn) {
	int i;

	/* operating on global variables */
	GetNWSLock(&lock);
	for(i = 0; i < MAX_NOTIFIES; i++) {
		if(disconnectFunctions[i] == NULL) {
			disconnectFunctions[i] = notifyFn;
			break;
		}
	}
	ReleaseNWSLock(&lock);
}


#define MAXPASSES 40
static pid_t passedPids[MAXPASSES];
static Socket passedSockets[MAXPASSES];


int
PassSocket(Socket *sock,
           pid_t child) {

  int i, childStat;

  /* Clean up any sockets previously passed to children who have exited. */
  for(i = 0; i < MAXPASSES; i++) {
    if(passedPids[i] != 0) {
      if((waitpid(passedPids[i], &childStat, WNOHANG) < 0) ||
         WIFEXITED(childStat)) {
        LOG1("PassSocket: Reclaiming connection %d\n", passedSockets[i]);
        (void)shutdown(passedSockets[i], 2);
        (void)close(passedSockets[i]);
        DoDisconnectNotification(passedSockets[i]);
        passedPids[i] = 0;
      }
    }
  }

  /* Record this socket in passedSockets and remove all other memory of it. */
  for(i = 0; i < MAXPASSES; i++) {
    if(passedPids[i] == 0) {
      LOG2("PassSocket: Passing connection %d to %d\n", *sock, child);
      passedPids[i] = child;
      passedSockets[i] = *sock;
      ClearSocket(*sock);
      *sock = NO_SOCKET;
      return(1);
    }
  }

  return(0);

}


/* here to be used by dnsutil.c (GetPeerName) */
int
IsPipe(Socket sd) {
	int ret = 0;

	GetNWSLock(&lock);
	if (FD_ISSET(sd, &connectedPipes)) {
		ret = 1;
	}
	ReleaseNWSLock(&lock);

	return ret;
}
	
int
RecvBytes(Socket sd,
          void *bytes,
          size_t byteSize,
          double timeOut) {

	double start = 0, myTimeOut = 0;
	int isPipe, tmp, done=1;
	char *nextByte;
	fd_set readFds;
	int recvd = 0, totalRecvd=0;
	struct timeval tout, *tv;
	void (*was)(int);

	/* let's see if we need to check disconnected socket */
	if (needDisconnect == 1) {
		needDisconnect = 0;
		(void)CloseDisconnections();
	}

	/* sanity check */
	if (sd < 0 || bytes == NULL) {
		WARN("RecvBytes: parameters out of range!\n");
		return 0;
	}

	/* connectedPipes is global */
	GetNWSLock(&lock);
	isPipe = FD_ISSET(sd, &connectedPipes);
	ReleaseNWSLock(&lock);

	FD_ZERO(&readFds);
	FD_SET(sd, &readFds);

	/* select the adaptive timeouts or the passed in one. */
	if (timeOut > 0) {
		myTimeOut = (int)timeOut;
		if (SignalAlarm(RecvTimeOut, &was) == 0) {
			WARN("Failed to set the alarm signal! Exiting\n");
			exit(1);
		}
		/* let's start the clock */
		start = CurrentTime();
	}

	for(nextByte=(char*)bytes; totalRecvd < byteSize; totalRecvd += recvd) {
		recvd = 0;
		UnlockMessageSystem();

		/* set the timeout if requested by the user */
		if (timeOut > 0) {
			myTimeOut = timeOut - (CurrentTime() - start);
			if (myTimeOut < 0) {
				done = 0;
				break;
			}
			tout.tv_usec = 0;
			tout.tv_sec = (int) myTimeOut;
			tv = &tout;
		} else {
			/* 0 is the special flag for don't use touts */
			tv = NULL;
		}

		tmp =  select(FD_SETSIZE, &readFds, NULL, NULL, tv);
		if (tmp == -1) {
			LockMessageSystem();

			/* just in case another call modify errno */
			GetNWSLock(&lock);
			tmp = errno;
			ReleaseNWSLock(&lock);

			/* if interrupted, let's try again */
			if(tmp == EINTR) {
				continue;
			}

			ERROR2("RecvBytes: select on %d failed (%s)\n", sd, strerror(tmp));
			done = 0;
			break;
		} else if (tmp == 0) {
			LockMessageSystem();

			/* timed out */
			ERROR1("RecvBytes: Socket %d timed out\n", sd);

			done = 0;
			break;
		}

		/* let's read the data */
		if (timeOut > 0) {
			myTimeOut = timeOut - (CurrentTime() - start);
			/* should always be > 0 since we didn't
			 * timed out on select */
			if (myTimeOut > 0) {
				SetRealTimer((unsigned int)myTimeOut);
			} else {
				ERROR1("RecvBytes: trying to set negative timeout on socket %d\n", sd);
				done = 0;
				break;
			}
		}

		if (isPipe) {
			/* pipe */
 			recvd = read(sd, nextByte, byteSize - totalRecvd);
		} else {
			/* socket */
 			recvd = recv(sd, nextByte, byteSize - totalRecvd, 0);
		}
		/* just in case another call modify errno */
		GetNWSLock(&lock);
		tmp = errno;
		ReleaseNWSLock(&lock);

		if (timeOut > 0) {
			RESETREALTIMER;
		}
		LockMessageSystem();

		if(recvd <= 0) {
			WARN2("RecvBytes: read failed errno:%d, %s\n", tmp, strerror(tmp));
			ERROR3("RecvBytes: socket %d failed after %d of %d bytes\n", sd, totalRecvd, byteSize);
			done = 0;
			break;
		}
		nextByte += recvd;
	}

	/* resetting the sigalarm */
	if (timeOut > 0) {
		SignalAlarm(was, NULL);
	}
	
	return totalRecvd;
}


/* it should be thread safe. */
int
SendBytes(	Socket sd,
		const void *bytes,
		size_t byteSize,
		double timeOut) {

	char *nextByte;
	int sent =0, totalSent = 0;
	int isPipe, tmp, done = 1;
	struct timeval tout, *tv;
	double start, myTimeOut = 0;
	void (*was)(int);


	/* let's see if we need to check disconnected socket */
	if (needDisconnect == 1) {
		needDisconnect = 0;
		(void)CloseDisconnections();
	}

	/* connectedPipes is global */
	GetNWSLock(&lock);
	isPipe = FD_ISSET(sd, &connectedPipes);
	ReleaseNWSLock(&lock);

	/* select the adaptive timeouts or the passed in one. */
	if (timeOut > 0) {
		myTimeOut = (double) timeOut;
		if (SignalAlarm(RecvTimeOut, &was) == 0) {
			ERROR("Failed to set the alarm signal! Exiting\n");
			exit(1);
		}
	}

	/* let's start the timer */
	start = CurrentTime();

	for(nextByte = (char*)bytes; totalSent < byteSize; totalSent += sent) {
		UnlockMessageSystem();

		/* set the timeout, and if we timed out get out */
		if (timeOut > 0) {
			/* 0 is the special flag for don't use touts */
			myTimeOut = timeOut - (CurrentTime() - start);
			if (myTimeOut < 0) {
				done = 0;
				break;
			} 
			tout.tv_usec = 0;
			tout.tv_sec = (int) myTimeOut;
			tv = &tout;
			SetRealTimer((unsigned int)myTimeOut);
		} 

		if (isPipe) {
			/* pipe */
			sent = write(sd, nextByte, byteSize - totalSent);
		} else {
			/* socket */
			sent = send(sd, nextByte, byteSize - totalSent, 0);
		}

		/* errno could be modified by another thread */
		GetNWSLock(&lock);
		tmp = errno;
		ReleaseNWSLock(&lock);

		if (timeOut > 0) {
			RESETREALTIMER;
		}

		LockMessageSystem();
		if(sent <= 0) {
			ERROR3("SendBytes: send on socket %d failed (errno=%d %s)\n", sd, tmp, strerror(tmp));
			done = 0;
			break;
		}
		nextByte += sent;
	}

	/* reset sigalalrm */
	if (timeOut > 0) {
		SignalAlarm(was, NULL);
	}

	return done;
}


void
SocketFailure(int sig) {
	HoldSignal(SIGPIPE);
	needDisconnect = 1;
	if(signal(SIGPIPE, SocketFailure) == SIG_ERR) {
		WARN("SocketFailure: error resetting signal\n");
	}
	ReleaseSignal(SIGPIPE);
}
