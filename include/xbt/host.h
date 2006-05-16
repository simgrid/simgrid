/* $Id$ */

/* host.h - host management functions                                       */

/* Copyright (c) 2006 Arnaud Legrand.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_HOST_H
#define XBT_HOST_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

typedef struct {  
   char *name;
   int port;
} s_xbt_host_t, *xbt_host_t;

xbt_host_t xbt_host_new(char *name, int port);
void xbt_host_free(xbt_host_t host);
void xbt_host_free_voidp(void *d);

SG_END_DECL()


#endif /* XBT_MISC_H */
