/* datadesc - describing the data to exchange                               */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_INTERFACE_H
#define GRAS_DATADESC_INTERFACE_H

XBT_PUBLIC(gras_datadesc_type_t) gras_datadesc_by_id(long int code);

/* to debug */
XBT_PUBLIC(void) gras_datadesc_type_dump(const gras_datadesc_type_t ddt);
XBT_PUBLIC(const char *) gras_datadesc_arch_name(int code);

/* compare two data type description */
XBT_PUBLIC(int)
gras_datadesc_type_cmp(const gras_datadesc_type_t d1,
                       const gras_datadesc_type_t d2);

/* Access function */
XBT_PUBLIC(int) gras_datadesc_size(gras_datadesc_type_t type);
/* Described data exchanges: direct use */
XBT_PUBLIC(int) gras_datadesc_memcpy(gras_datadesc_type_t type, void *src,
                                     void *dst);
XBT_PUBLIC(void) gras_datadesc_send(gras_socket_t sock,
                                    gras_datadesc_type_t type, void *src);
XBT_PUBLIC(void) gras_datadesc_recv(gras_socket_t sock,
                                    gras_datadesc_type_t type, int r_arch,
                                    void *dst);

/* Described data exchanges: IDL compilation FIXME: not implemented*/
     void gras_datadesc_gen_cpy(gras_datadesc_type_t type, void *src,
                                void **dst);
     void gras_datadesc_gen_send(gras_socket_t sock,
                                 gras_datadesc_type_t type, void *src);
     void gras_datadesc_gen_recv(gras_socket_t sock,
                                 gras_datadesc_type_t type, int r_arch,
                                 void *dst);


#endif /* GRAS_DATADESC_INTERFACE_H */
