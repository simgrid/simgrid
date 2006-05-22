/* $Id$ */

/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmrpc.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(MatMult,"Messages specific to this example");

/* register messages which may be sent and their payload
   (common to client and server) */
void mmrpc_register_messages(void) {
  gras_datadesc_type_t matrix_type, request_type;

  matrix_type=gras_datadesc_by_symbol(s_matrix);
  request_type=gras_datadesc_array_fixed("matrix_t[2]",matrix_type,2);
  
  gras_msgtype_declare("answer", matrix_type);
  gras_msgtype_declare("request", request_type);
}

void mat_dump(matrix_t *mat, const char* name) {
  int i,j;

  printf(">>> Matrix %s dump (%d x %d)\n",name,mat->lines,mat->rows);
  for (i=0; i<mat->lines; i++) {
    printf("  ");
    for (j=0; j<mat->rows; j++)
      printf(" %.2f",mat->ctn[i*mat->rows + j]);
    printf("\n");
  }
  printf("<<< end_of_matrix %s dump\n",name);
}



