/* $Id$ */

/* trp_tcp_server: Server of a test case for the tcp transport.             */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
 
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <stdio.h>
#include "gras.h"
#include "gras/Transport/transport_interface.h"

XBT_LOG_NEW_CATEGORY(test,"Logging for this test");

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc,char *argv[]) {
  gras_socket_t sock;
  char data_recv[256];

  gras_init(&argc,argv);

  fprintf(stderr,"===[SERVER]=== Create the socket\n");
  sock = gras_socket_server_from_file("-");

  fprintf(stderr,"===[SERVER]=== Waiting for the data\n");
  gras_trp_recv(sock,data_recv, sizeof(data_recv));
  fprintf(stderr,"===[SERVER]=== Got '%s'.\n", data_recv);

  fprintf(stderr,"===[SERVER]=== Exiting successfully\n");
  gras_socket_close(sock);

  gras_exit();
  return 0;
}
