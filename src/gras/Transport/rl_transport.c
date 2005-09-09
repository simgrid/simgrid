/* $Id$ */

/* rl_transport - RL specific functions for transport                       */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "portable.h"
#include "gras/Transport/transport_private.h"
XBT_LOG_EXTERNAL_CATEGORY(transport);
XBT_LOG_DEFAULT_CATEGORY(transport);

/**
 * gras_trp_select:
 *
 * Returns the next socket to service because it receives a message.
 *
 * if timeout<0, we ought to implement the adaptative timeout (FIXME)
 *
 * if timeout=0, do not wait for new message, only handle the ones already there.
 *
 * if timeout>0 and no message there, wait at most that amount of time before giving up.
 */
gras_socket_t gras_trp_select(double timeout) {
  xbt_dynar_t sockets= ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->sockets;
  int done = -1;
  double wakeup = gras_os_time() + timeout;
  double now = 0;
  /* nextToService used to make sure socket with high number do not starve */
  /*  static int nextToService = 0; */
  struct timeval tout, *p_tout;

  int max_fds=0; /* first arg of select: number of existing sockets */
  /* but accept() of winsock returns sockets bigger than the limit, so don't bother 
     with this tiny optimisation on BillWare */
  fd_set FDS;
  int ready; /* return of select: number of socket ready to be serviced */
  int fd_setsize; /* FD_SETSIZE not always defined. Get this portably */

  gras_socket_t sock_iter; /* iterating over all sockets */
  int cursor;              /* iterating over all sockets */

   
  /* Compute FD_SETSIZE */
#ifdef HAVE_SYSCONF
   fd_setsize = sysconf( _SC_OPEN_MAX );
#else
#  ifdef HAVE_GETDTABLESIZE 
   fd_setsize = getdtablesize();
#  else
   fd_setsize = FD_SETSIZE;
#  endif /* !USE_SYSCONF */
#endif

  while (done == -1) {
    if (timeout > 0) { /* did we timeout already? */
      now = gras_os_time();
      if (now == -1 || now >= wakeup) {
	done = 0;	/* didn't find anything */
	break;
      }
    }

    /* construct the set of socket to ear from */
    FD_ZERO(&FDS);
    max_fds = -1;
    xbt_dynar_foreach(sockets,cursor,sock_iter) {
      if (sock_iter->incoming) {
	DEBUG1("Considering socket %d for select",sock_iter->sd);
#ifndef HAVE_WINSOCK_H
	if (max_fds < sock_iter->sd)
	  max_fds = sock_iter->sd;
#endif
	FD_SET(sock_iter->sd, &FDS);
      } else {
	DEBUG1("Not considering socket %d for select",sock_iter->sd);
      }
    }

    if (max_fds == -1) {
       if (timeout > 0) {
	  DEBUG1("No socket to select onto. Sleep %f sec instead.",timeout);
	  gras_os_sleep(timeout);
	  THROW1(timeout_error,0,"No socket to select onto. Sleep %f sec instead",timeout);
       } else {
	  DEBUG0("No socket to select onto. Return directly.");
	  THROW0(timeout_error,0, "No socket to select onto. Return directly.");
       }
    }

#ifndef HAVE_WINSOCK_H
    /* we cannot have more than FD_SETSIZE sockets 
       ... but with WINSOCK which returns sockets higher than the limit (killing this optim) */
    if (++max_fds > fd_setsize && fd_setsize > 0) {
      WARN1("too many open sockets (%d).",max_fds);
      done = 0;
      break;
    }
#else
    max_fds = fd_setsize;
#endif

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
	THROW3(system_error,EINVAL,"invalid select: nb fds: %d, timeout: %d.%d",
	       max_fds, (int)tout.tv_sec,(int) tout.tv_usec);
      case ENOMEM: 
	xbt_assert0(0,"Malloc error during the select");
      default:
	THROW2(system_error,errno,"Error during select: %s (%d)",
	       strerror(errno),errno);
      }
      THROW_IMPOSSIBLE;
    } else if (ready == 0) {
      continue;	 /* this was a timeout */
    }

    xbt_dynar_foreach(sockets,cursor,sock_iter) { 
       if(!FD_ISSET(sock_iter->sd, &FDS)) { /* this socket is not ready */
	continue;
       }
       
       /* Got a socket to serve */
       ready--;

       if (   sock_iter->accepting
	   && sock_iter->plugin->socket_accept) { 
	 /* not a socket but an ear. accept on it and serve next socket */
	 gras_socket_t accepted=NULL;
	 
	 accepted = (sock_iter->plugin->socket_accept)(sock_iter);
	 DEBUG2("accepted=%p,&accepted=%p",accepted,&accepted);
	 accepted->meas = sock_iter->meas;
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
	   XBT_OUT;
	   return sock_iter;
#if 0
	 }
#endif
       }

       
       /* if we're here, the socket we found wasn't really ready to be served */
       if (ready == 0) /* exausted all sockets given by select. Request new ones */
	 break; 
    }

  }

  XBT_OUT;
  return NULL;
}

void gras_trp_sg_setup(gras_trp_plugin_t plug) {
  THROW0(mismatch_error,0,"No SG transport on live platforms");
}

