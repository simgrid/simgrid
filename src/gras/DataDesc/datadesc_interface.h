/* $Id$ */

/* datadesc - describing the data to exchange                               */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004 Olivier Aumage, Martin Quinson. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_INTERFACE_H
#define GRAS_DATADESC_INTERFACE_H

xbt_error_t gras_datadesc_by_id  (long int code,
				   gras_datadesc_type_t *type);

/* to debug */
void gras_datadesc_type_dump(const gras_datadesc_type_t ddt);
const char *gras_datadesc_arch_name(int code);

/* compare two data type description */
int
gras_datadesc_type_cmp(const gras_datadesc_type_t d1,
		       const gras_datadesc_type_t d2);

/* Access function */
int  gras_datadesc_size(gras_datadesc_type_t type);
/* Described data exchanges */
xbt_error_t gras_datadesc_cpy(gras_datadesc_type_t type, void *src, void **dst);
xbt_error_t gras_datadesc_send(gras_socket_t sock, gras_datadesc_type_t type, void *src);
xbt_error_t gras_datadesc_recv(gras_socket_t sock, gras_datadesc_type_t type,
				int r_arch, void *dst);

   
#endif /* GRAS_DATADESC_INTERFACE_H */
