/* $Id$ */

/* trp_tcp_server: Server of a test case for the tcp transport.             */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>
#include "../Transport/transport_interface.h"

//GRAS_LOG_NEW_DEFAULT_CATEGORY(test);

int main(int argc,char *argv[]) {
  gras_socket_t *sock, *conn;
  gras_error_t errcode;
  char data_recv[256];

  gras_init_defaultlog(argc,argv,"trp.thresh=debug");

  fprintf(stderr,"===[SERVER]=== Create the socket\n");
  TRYFAIL(gras_socket_server(55555,&sock));

  fprintf(stderr,"===[SERVER]=== Waiting for incomming connexions\n");
  TRYFAIL(gras_trp_select(60,&conn));

  fprintf(stderr,"===[SERVER]=== Contacted ! Waiting for the data\n");
  TRYFAIL(gras_trp_chunk_recv(conn,data_recv, sizeof(data_recv)));
  fprintf(stderr,"===[SERVER]=== Got '%s'. Send it back.\n", data_recv);
  TRYFAIL(gras_trp_chunk_send(conn,data_recv, sizeof(data_recv)));
  gras_socket_close(&conn);

  fprintf(stderr,"===[SERVER]=== Exiting successfully\n");
  gras_socket_close(&sock);

  return 0;
}
