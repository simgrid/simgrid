/* $Id$ */

/* datadesc - describing the data to exchange                               */

/* module's public interface exported within GRAS, but not to end user.     */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_INTERFACE_H
#define GRAS_DATADESC_INTERFACE_H

void gras_datadesc_init(void);
void gras_datadesc_exit(void);

gras_error_t gras_datadesc_by_id  (long int code,
				   gras_datadesc_type_t **type);

/* to debug */
void gras_datadesc_type_dump(const gras_datadesc_type_t *ddt);
const char *gras_datadesc_arch_name(int code);

/* compare two data type description */
int
gras_datadesc_type_cmp(const gras_datadesc_type_t *d1,
		       const gras_datadesc_type_t *d2);

/* Access function */
int  gras_datadesc_size(gras_datadesc_type_t *type);
/* Described data exchanges */
gras_error_t 
gras_datadesc_cpy(gras_datadesc_type_t *type, void *src, void **dst);
gras_error_t 
gras_datadesc_send(gras_socket_t *sock, gras_datadesc_type_t *type, void *src);
gras_error_t
gras_datadesc_recv(gras_socket_t *sock, gras_datadesc_type_t *type, 
		   int r_arch, void *dst);

/* Indicate (lack of) interest in datatype */
void gras_datadesc_ref(gras_datadesc_type_t *type);
void gras_datadesc_unref(gras_datadesc_type_t *type);
   
#endif /* GRAS_DATADESC_INTERFACE_H */
