/* $Id$ */

/* trp (transport) - send/receive a bunch of bytes                          */

/* This file implements the public interface of this module, exported to the*/
/*  other modules of GRAS, but not to the end user.                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

/* each plugin implements the socket the way it wants */
typedef void gras_trp_sock_t;

/* A plugin type */
typedef struct gras_trp_plugin_ gras_trp_plugin_t;

/* Module, and get plugin by name */
gras_error_t gras_trp_init(void);

void         gras_trp_exit(void);

gras_error_t gras_trp_plugin_get_by_name(const char *name,
					 gras_trp_plugin_t **dst);



#endif /* GRAS_TRANSPORT_H */
