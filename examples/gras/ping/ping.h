/* $Id$ */

/* ping - ping/pong demo of GRAS features                                   */

/* Copyright (c) 2003, 2004, 2005 Martin Quinson. All rights reserved.      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef PING_COMMON_H
#define PING_COMMON_H

#include "gras.h"

/* register messages which may be sent (common to client and server) */
void ping_register_messages(void);

/* Function prototypes */
int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

#endif
