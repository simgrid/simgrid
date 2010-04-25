/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmrpc.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(MatMult, "Messages specific to this example");

/* register messages which may be sent and their payload
   (common to client and server) */
void mmrpc_register_messages(void)
{
  gras_datadesc_type_t matrix_type, request_type;

  matrix_type = gras_datadesc_matrix(gras_datadesc_by_name("double"), NULL);
  request_type =
    gras_datadesc_array_fixed("s_matrix_t(double)[2]", matrix_type, 2);

  gras_msgtype_declare("answer", matrix_type);
  gras_msgtype_declare("request", request_type);
}
