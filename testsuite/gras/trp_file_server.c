/* $Id$ */

/* trp_tcp_server: Server of a test case for the tcp transport.             */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "gras.h"
#include "gras/Transport/transport_interface.h"

/*XBT_LOG_NEW_DEFAULT_CATEGORY(test);*/

int main(int argc,char *argv[]) {
  gras_socket_t sock, conn;
  xbt_error_t errcode;
  char data_recv[256];

  gras_init(&argc,argv,"trp.thresh=debug");

  fprintf(stderr,"===[SERVER]=== Create the socket\n");
  TRYFAIL(gras_socket_server_from_file("-",&sock));

  fprintf(stderr,"===[SERVER]=== Waiting for incomming connexions\n");
  TRYFAIL(gras_trp_select(60,&conn));

  fprintf(stderr,"===[SERVER]=== Contacted ! Waiting for the data\n");
  TRYFAIL(gras_trp_chunk_recv(conn,data_recv, sizeof(data_recv)));
  fprintf(stderr,"===[SERVER]=== Got '%s'.\n", data_recv);

  fprintf(stderr,"===[SERVER]=== Exiting successfully\n");
  gras_socket_close(sock);

  xbt_exit();
  return 0;
}
