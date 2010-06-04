#ifndef __XBT_WIN32_CONFIG_H__
#define __XBT_WIN32_CONFIG_H__


/* config.h - simgrid config selection for windows platforms. */

/* Copyright (c) 2006, 2007, 2008, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
 * config selection. 
*/
#include <win32/compiler/select_compiler_features.h>


#if defined(_XBT_BORLAND_COMPILER)
# include <win32/compiler/borland.h>

#elif defined(__GNUC__)
  /* data comes from autoconf when using gnuc (cross-compiling?) */
# include "gras_config.h"
typedef unsigned int uint32_t;

#elif defined(_XBT_VISUALC_COMPILER)
# include <win32/compiler/visualc.h>
# else
# error "Unknown compiler - please report the problems to the main simgrid mailing list (http://gforge.inria.fr/mail/?group_id=12)"
#endif

typedef int socklen_t;
#define tcp_read( s, buf, len )  	recv( s, buf, len, 0 )
#define tcp_write( s, buf, len )	send( s, buf, len, 0 )
#define ioctl( s, c, a )        	ioctlsocket( (s), (c), (a) )
#define ioctl_t						u_long
#define AC_SOCKET_INVALID        	((unsigned int) ~0)

#ifdef SD_BOTH
	#define tcp_close(s)	(shutdown( s, SD_BOTH ), closesocket(s))
#else
	#define tcp_close( s )	closesocket( s )
#endif

#ifndef _XBT_VISUALC_COMPILER
	#ifndef EWOULDBLOCK
		#define EWOULDBLOCK WSAEWOULDBLOCK
	#endif
	#ifndef EINPROGRESS
		#define EINPROGRESS WSAEINPROGRESS
	#endif
	#ifndef ETIMEDOUT
		#define ETIMEDOUT   WSAETIMEDOUT
	#endif
#endif



#ifdef sock_errno
	#undef  sock_errno
#endif

#define sock_errno         WSAGetLastError()

#ifdef sock_errstr
	#undef  sock_errstr
#endif

#define sock_errstr(err)   gras_wsa_err2string(err)

const char *gras_wsa_err2string(int errcode);

#ifdef S_IRGRP
	#undef S_IRGRP
#endif

#define S_IRGRP 0

#ifdef S_IWGRP
	#undef S_IWGRP
#endif

#define S_IWGRP 0



#endif /* #ifndef __XBT_WIN32_CONFIG_H__ */
