/* $Id$ */

/* rl_transport - RL specific functions for transport                       */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Transport/transport_private.h"
GRAS_LOG_EXTERNAL_CATEGORY(transport);
GRAS_LOG_DEFAULT_CATEGORY(transport);



/**
 * gras_trp_select:
 *
 * Returns the next socket to service having a message awaiting.
 *
 * if timeout<0, we ought to implement the adaptative timeout (FIXME)
 *
 * if timeout=0, do not wait for new message, only handle the ones already there.
 *
 * if timeout>0 and no message there, wait at most that amount of time before giving up.
 */
gras_error_t 
gras_trp_select(double timeout,
		gras_socket_t **dst) {

  gras_error_t errcode;
  int done = -1;
  double wakeup = gras_time() + 1000000*timeout;
  double now = 0;
  /* nextToService used to make sure socket with high number do not starve */
  //  static int nextToService = 0;
  struct timeval tout, *p_tout;

  int max_fds=0; /* first arg of select: number of existing sockets */
  fd_set FDS;
  int ready; /* return of select: number of socket ready to be serviced */

  gras_socket_t *sock_iter; /* iterating over all sockets */
  int cursor;               /* iterating over all sockets */

  *dst=NULL;
  while (done == -1) {
    if (timeout > 0) { /* did we timeout already? */
      now = gras_time();
      if (now == -1 || now >= wakeup) {
	done = 0;	/* didn't find anything */
	break;
      }
    }

    /* construct the set of socket to ear from */
    FD_ZERO(&FDS);
    gras_dynar_foreach(_gras_trp_sockets,cursor,sock_iter) {
      if (sock_iter->incoming) {
	if (max_fds < sock_iter->sd)
	  max_fds = sock_iter->sd;
	FD_SET(sock_iter->sd, &FDS);
      } else {
	DEBUG1("Not considering socket %d for select",sock_iter->sd);
      }
    }

    /* we cannot have more than FD_SETSIZE sockets */
    if (++max_fds > FD_SETSIZE) {
      WARN0("too many open sockets.");
      done = 0;
      break;
    }

    if (timeout > 0) { 
      /* set the timeout */
      tout.tv_sec = (unsigned long)((wakeup - now)/1000000);
      tout.tv_usec = (unsigned long)(wakeup - now) % 1000000;
      p_tout = &tout;
    } else if (timeout == 0) {
      /* polling only */
      tout.tv_sec = 0;
      tout.tv_usec = 0;
      p_tout = &tout;
      /* we just do one loop around */
      done = 0;
    } else { 
      /* no timeout: good luck! */
      p_tout = NULL;
    }

    DEBUG1("Selecting over %d socket(s)", max_fds-1);
    ready = select(max_fds, &FDS, NULL, NULL, p_tout);
    if (ready == -1) {
      switch (errno) {
      case  EINTR: /* a signal we don't care about occured. we don't care */
	/* if we cared, we would have set an handler */
	continue;
      case EINVAL: /* invalid value */
	RAISE3(system_error,"invalid select: nb fds: %d, timeout: %d.%d",
	       max_fds, (int)tout.tv_sec,(int) tout.tv_usec);
      case ENOMEM: 
	RAISE_MALLOC;
      default:
	RAISE2(system_error,"Error during select: %s (%d)",
	       strerror(errno),errno);
      }
      RAISE_IMPOSSIBLE;
    } else if (ready == 0) {
      continue;	 /* this was a timeout */
    }

    gras_dynar_foreach(_gras_trp_sockets,cursor,sock_iter) { 
       if(!FD_ISSET(sock_iter->sd, &FDS)) { /* this socket is not ready */
	continue;
       }
       
       /* Got a socket to serve */
       ready--;

       if (   sock_iter->accepting
	   && sock_iter->plugin->socket_accept) { 
	 /* not a socket but an ear. accept on it and serve next socket */
	 gras_socket_t *accepted;

	 TRY(sock_iter->plugin->socket_accept(sock_iter,&accepted));
	 TRY(gras_dynar_push(_gras_trp_sockets,&accepted));
       } else {
#if 0 
       FIXME: this fails of files. quite logical
	 /* Make sure the socket is still alive by reading the first byte */
	 char lookahead;
	 int recvd;

	 recvd = recv(sock_iter->sd, &lookahead, 1, MSG_PEEK);
	 if (recvd < 0) {
	   WARN2("socket %d failed: %s", sock_iter->sd, strerror(errno));
	   /* done with this socket */
	   gras_socket_close(&sock_iter);
	   cursor--;
	 } else if (recvd == 0) {
	   /* Connection reset (=closed) by peer. */
	   DEBUG1("Connection %d reset by peer", sock_iter->sd);
	   gras_socket_close(&sock_iter); 
	   cursor--; 
	 } else { 
#endif
	   /* Got a suited socket ! */
	   *dst = sock_iter;
	   return no_error;
#if 0
	 }
#endif
       }

       
       /* if we're here, the socket we found wasn't really ready to be served */
       if (ready == 0) /* exausted all sockets given by select. Request new ones */
	 break; 
    }

  }

  return timeout_error;
}
