/* $Id$ */

/* xbt_host_t management functions                                          */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/host.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host,xbt,"Host management");

/** \brief constructor */
xbt_host_t xbt_host_new(const char *name, int port)  {
   xbt_host_t res=xbt_new(s_xbt_host_t, 1);
   res->name = xbt_strdup(name);
   res->port = port;
   return res;
}

xbt_host_t xbt_host_copy(xbt_host_t h) {
   return xbt_host_new(h->name,h->port);
}

/** \brief constructor. Argument should be of form '<hostname>:<port>'. */
xbt_host_t xbt_host_from_string(const char *hostport)  {
   xbt_host_t res=xbt_new(s_xbt_host_t, 1);
   char *name=xbt_strdup(hostport);
   char *port_str=strchr(name,':');
   xbt_assert1(port_str,"argument of xbt_host_from_string should be of form <hostname>:<port>, it's '%s'", hostport);
   *port_str='\0';
   port_str++;
   
   res->name = xbt_strdup(name); /* it will be shorter now that we cut the port */
   res->port = atoi(port_str);
   free(name);
   return res;
}

/** \brief destructor */
void xbt_host_free(xbt_host_t host) {
   if (host) {
      if (host->name) free(host->name);
      free(host);
   }
}

/** \brief Freeing function for dynars of xbt_host_t */
void xbt_host_free_voidp(void *d) {
   xbt_host_free( (xbt_host_t) *(void**)d );
}
