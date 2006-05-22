/* $Id$ */

/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MMRPC_H
#define MMRPC_H

#include "gras.h"

#define MATSIZE 128

GRAS_DEFINE_TYPE(s_matrix,
struct s_matrix {
  int lines;
  int rows;
  double *ctn GRAS_ANNOTE(size, lines*rows);
};)
typedef struct s_matrix matrix_t;

void mat_dump(matrix_t *mat, const char* name);

/* register messages which may be sent and their payload
   (common to client and server) */
void mmrpc_register_messages(void);

/* Function prototypes */
int server (int argc,char *argv[]);
int client (int argc,char *argv[]);

#endif /* MMRPC_H */
