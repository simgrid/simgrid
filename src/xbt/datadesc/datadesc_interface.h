/* datadesc - describing the data to exchange                               */

/* module's public interface exported within XBT, but not to end user.     */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_DATADESC_INTERFACE_H
#define XBT_DATADESC_INTERFACE_H

#include "xbt/datadesc.h"
#include "xbt/misc.h"
#include "xbt/socket.h"

XBT_PUBLIC(xbt_datadesc_type_t) xbt_datadesc_by_id(long int code);

/* to debug */
XBT_PUBLIC(void) xbt_datadesc_type_dump(const xbt_datadesc_type_t ddt);
XBT_PUBLIC(const char *) xbt_datadesc_arch_name(int code);

/* compare two data type description */
XBT_PUBLIC(int)
xbt_datadesc_type_cmp(const xbt_datadesc_type_t d1,
                       const xbt_datadesc_type_t d2);

/* Access function */
XBT_PUBLIC(int) xbt_datadesc_size(xbt_datadesc_type_t type);
/* Described data exchanges: direct use */
XBT_PUBLIC(int) xbt_datadesc_memcpy(xbt_datadesc_type_t type, void *src,
                                     void *dst);
XBT_PUBLIC(void) xbt_datadesc_send(xbt_socket_t sock,
                                    xbt_datadesc_type_t type, void *src);
XBT_PUBLIC(void) xbt_datadesc_recv(xbt_socket_t sock,
                                    xbt_datadesc_type_t type, int r_arch,
                                    void *dst);

/* Described data exchanges: IDL compilation FIXME: not implemented*/
void xbt_datadesc_gen_cpy(xbt_datadesc_type_t type, void *src,
                           void **dst);
void xbt_datadesc_gen_send(xbt_socket_t sock,
                            xbt_datadesc_type_t type, void *src);
void xbt_datadesc_gen_recv(xbt_socket_t sock,
                            xbt_datadesc_type_t type, int r_arch,
                            void *dst);

#endif                          /* XBT_DATADESC_INTERFACE_H */
