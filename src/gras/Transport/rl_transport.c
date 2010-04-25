/* rl_transport - RL specific functions for transport                       */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "portable.h"
#include "gras/Transport/transport_private.h"
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_trp);

/* check transport_private.h for an explanation of this variable */
gras_socket_t _gras_lastly_selected_socket = NULL;

/**
 * gras_trp_select:
 *
 * Returns the next socket to service because it receives a message.
 *
 * if timeout<0, we ought to implement the adaptative timeout (FIXME)
 *
 *
 * if timeout>0 and no message there, wait at most that amount of time before giving up.
 */
gras_socket_t gras_trp_select(double timeout)
{
  xbt_dynar_t sockets =
    ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->sockets;
  int done = -1;
  double wakeup = gras_os_time() + timeout;
  double now = 0;
  /* nextToService used to make sure socket with high number do not starve */
  /*  static int nextToService = 0; */
  struct timeval tout, *p_tout;

  int max_fds = 0;              /* first arg of select: number of existing sockets */
  /* but accept() of winsock returns sockets bigger than the limit, so don't bother 
     with this tiny optimisation on BillWare */
  fd_set FDS;
  int ready;                    /* return of select: number of socket ready to be serviced */
  static int fd_setsize = -1;   /* FD_SETSIZE not always defined. Get this portably */

  gras_socket_t sock_iter;      /* iterating over all sockets */
  unsigned int cursor;          /* iterating over all sockets */

  /* Check whether there is more data to read from the socket we selected last time.
     This can happen with tcp buffered sockets since we try to get as much data as we can for them */
  if (_gras_lastly_selected_socket && _gras_lastly_selected_socket->moredata) {
    VERB0
      ("Returning _gras_lastly_selected_socket since there is more data on it");
    return _gras_lastly_selected_socket;
  }

  /* Compute FD_SETSIZE on need */
  if (fd_setsize < 0) {
#ifdef HAVE_SYSCONF
    fd_setsize = sysconf(_SC_OPEN_MAX);
#else
#  ifdef HAVE_GETDTABLESIZE
    fd_setsize = getdtablesize();
#  else
    fd_setsize = FD_SETSIZE;
#  endif /* !USE_SYSCONF */
#endif
  }

  while (done == -1) {
    if (timeout > 0) {          /* did we timeout already? */
      now = gras_os_time();
      DEBUG2("wakeup=%f now=%f", wakeup, now);
      if (now == -1 || now >= wakeup) {
        /* didn't find anything; no need to update _gras_lastly_selected_socket since its moredata is 0 (or we would have returned it directly) */
        THROW1(timeout_error, 0,
               "Timeout (%f) elapsed with selecting for incomming connexions",
               timeout);
      }
    }

    /* construct the set of socket to ear from */
    FD_ZERO(&FDS);
    max_fds = -1;
    xbt_dynar_foreach(sockets, cursor, sock_iter) {
      if (!sock_iter->valid)
        continue;

      if (sock_iter->incoming) {
        DEBUG1("Considering socket %d for select", sock_iter->sd);
#ifndef HAVE_WINSOCK_H
        if (max_fds < sock_iter->sd)
          max_fds = sock_iter->sd;
#else
        max_fds = 0;

#endif
        FD_SET(sock_iter->sd, &FDS);
      } else {
        DEBUG1("Not considering socket %d for select", sock_iter->sd);
      }
    }


    if (max_fds == -1) {
      if (timeout > 0) {
        DEBUG1("No socket to select onto. Sleep %f sec instead.", timeout);
        gras_os_sleep(timeout);
        THROW1(timeout_error, 0,
               "No socket to select onto. Sleep %f sec instead", timeout);
      } else {
        DEBUG0("No socket to select onto. Return directly.");
        THROW0(timeout_error, 0,
               "No socket to select onto. Return directly.");
      }
    }
#ifndef HAVE_WINSOCK_H
    /* we cannot have more than FD_SETSIZE sockets 
       ... but with WINSOCK which returns sockets higher than the limit (killing this optim) */
    if (++max_fds > fd_setsize && fd_setsize > 0) {
      WARN1("too many open sockets (%d).", max_fds);
      done = 0;
      break;
    }
#else
    max_fds = fd_setsize;
#endif

    tout.tv_sec = tout.tv_usec = 0;
    if (timeout > 0) {
      /* set the timeout */
      tout.tv_sec = (unsigned long) (wakeup - now);
      tout.tv_usec =
        ((wakeup - now) - ((unsigned long) (wakeup - now))) * 1000000;
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

    DEBUG2("Selecting over %d socket(s); timeout=%f", max_fds - 1, timeout);
    ready = select(max_fds, &FDS, NULL, NULL, p_tout);
    DEBUG1("select returned %d", ready);
    if (ready == -1) {
      switch (errno) {
      case EINTR:              /* a signal we don't care about occured. we don't care */
        /* if we cared, we would have set an handler */
        continue;
      case EINVAL:             /* invalid value */
        THROW3(system_error, EINVAL,
               "invalid select: nb fds: %d, timeout: %d.%d", max_fds,
               (int) tout.tv_sec, (int) tout.tv_usec);
      case ENOMEM:
        xbt_die("Malloc error during the select");
      default:
        THROW2(system_error, errno, "Error during select: %s (%d)",
               strerror(errno), errno);
      }
      THROW_IMPOSSIBLE;
    } else if (ready == 0) {
      continue;                 /* this was a timeout */
    }

    xbt_dynar_foreach(sockets, cursor, sock_iter) {
      if (!FD_ISSET(sock_iter->sd, &FDS)) {     /* this socket is not ready */
        continue;
      }

      /* Got a socket to serve */
      ready--;

      if (sock_iter->accepting && sock_iter->plugin->socket_accept) {
        /* not a socket but an ear. accept on it and serve next socket */
        gras_socket_t accepted = NULL;

        /* release mutex before accept; it will change the sockets dynar, so we have to break the foreach asap */
        xbt_dynar_cursor_unlock(sockets);
        accepted = (sock_iter->plugin->socket_accept) (sock_iter);

        DEBUG2("accepted=%p,&accepted=%p", accepted, &accepted);
        accepted->meas = sock_iter->meas;
        break;

      } else {
        /* Make sure the socket is still alive by reading the first byte */
        int recvd;

        if (sock_iter->recvd) {
          /* Socket wasn't used since last time! Don't bother checking whether it's still alive */
          recvd = 1;
        } else {
          recvd = read(sock_iter->sd, &sock_iter->recvd_val, 1);
        }

        if (recvd < 0) {
          WARN2("socket %d failed: %s", sock_iter->sd, strerror(errno));
          /* done with this socket; remove it and break the foreach since it will change the dynar */
          xbt_dynar_cursor_unlock(sockets);
          gras_socket_close(sock_iter);
          break;
        } else if (recvd == 0) {
          /* Connection reset (=closed) by peer. */
          DEBUG1("Connection %d reset by peer", sock_iter->sd);
          sock_iter->valid = 0; /* don't close it. User may keep references to it */
        } else {
          /* Got a suited socket ! */
          XBT_OUT;
          sock_iter->recvd = 1;
          DEBUG3("Filled little buffer (%c %x %d)", sock_iter->recvd_val,
                 sock_iter->recvd_val, recvd);
          _gras_lastly_selected_socket = sock_iter;
          /* break sync dynar iteration */
          xbt_dynar_cursor_unlock(sockets);
          return sock_iter;
        }
      }

      /* if we're here, the socket we found wasn't really ready to be served */
      if (ready == 0) {         /* exausted all sockets given by select. Request new ones */

        xbt_dynar_cursor_unlock(sockets);
        break;
      }
    }

  }

  /* No socket found. Maybe we had timeout=0 and nothing to do */
  DEBUG0("TIMEOUT");
  THROW0(timeout_error, 0, "Timeout");
}

void gras_trp_sg_setup(gras_trp_plugin_t plug)
{
  THROW0(mismatch_error, 0, "No SG transport on live platforms");
}
