/* $Id$ */

/* portable -- header loading to write portable code                         */
/* loads much more stuff than sysdep.h since the latter is in public interface*/

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                   */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PORTABLE_H
#define GRAS_PORTABLE_H

#include "gras_config.h"

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

/****
 **** Networking 
 ****/


#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#  include <netinet/in.h>   /* sometimes required for #include <arpa/inet.h> */
#  include <netinet/tcp.h>  /* TCP_NODELAY */
#  include <netdb.h>        /* getprotobyname() */
#  include <arpa/inet.h>    /* inet_ntoa() */
# endif

#ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
#  include <ws2tcpip.h>  /* socklen_t, but doubtful */
#  ifndef HAVE_WINSOCK_H
#    define HAVE_WINSOCK_H
#  endif
#elif HAVE_WINSOCK_H
#  include <winsock.h>
#endif

#ifdef HAVE_WINSOCK_H
#       define tcp_read( s, buf, len )  recv( s, buf, len, 0 )
#       define tcp_write( s, buf, len ) send( s, buf, len, 0 )
#       define ioctl( s, c, a )         ioctlsocket( (s), (c), (a) )
#       define ioctl_t                          u_long
#       define AC_SOCKET_INVALID        ((unsigned int) ~0)

#       ifdef SD_BOTH
#               define tcp_close( s )   (shutdown( s, SD_BOTH ), closesocket( s ))
#       else
#               define tcp_close( s )           closesocket( s )
#       endif

#       define EWOULDBLOCK WSAEWOULDBLOCK
#       define EINPROGRESS WSAEINPROGRESS
#       define ETIMEDOUT   WSAETIMEDOUT

#       undef  sock_errno
#       undef  sock_errstr
#       define sock_errno      WSAGetLastError()
#       define sock_errstr     gras_wsa_err2string(WSAGetLastError())

const char *gras_wsa_err2string(int errcode);

#       define S_IRGRP 0
#       define S_IWGRP 0

#else
#       define tcp_read( s, buf, len)   read( s, buf, len )
#       define tcp_write( s, buf, len)  write( s, buf, len )
#       define sock_errno      errno
#       define sock_errstr     strerror(errno)

#       ifdef SHUT_RDWR
#               define tcp_close( s )   (shutdown( s, SHUT_RDWR ), close( s ))
#       else
#               define tcp_close( s )   close( s )
#       endif
#endif /* windows or unix ? */

/****
 **** File handling
 ****/

#include <fcntl.h>

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifndef O_BINARY
#  define O_BINARY 0
#endif

/****
 **** Time handling
 ****/

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef _WIN32
#define sleep _sleep /* else defined in stdlib.h */
#endif

/****
 **** Contexts
 ****/

#ifdef USE_UCONTEXT
# ifndef S_SPLINT_S /* This header drives splint into the wall */
#   include <ucontext.h>
# endif 
#endif

#ifdef _WIN32
#  include "xbt/context_win32.h" /* Manual reimplementation for prehistoric platforms */
#endif

/**
 ** What is needed to protect solaris's printf from ever seing NULL associated to a %s format
 ** (without adding an extra check on linux :)
 **/

#ifdef PRINTF_NULL_WORKING
#  define PRINTF_STR(a) (a)
#else
#  define PRINTF_STR(a) (a)?:"(null)"
#endif
  

#endif /* GRAS_PORTABLE_H */
