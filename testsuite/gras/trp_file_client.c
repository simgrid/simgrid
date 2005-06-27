/* $Id$ */

/* trp_tcp_client: Client of a test case for the tcp transport.             */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "gras.h"
#include "gras/Transport/transport_interface.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging for this test");

int main(int argc,char *argv[]) {
  gras_socket_t sock;
  xbt_error_t errcode;
  char data_send[256];

  memset(data_send,0,sizeof(data_send));
  gras_init(&argc,argv);

  fprintf(stderr,"===[CLIENT]=== Contact the server\n");
  TRYFAIL(gras_socket_client_from_file("-",&sock));

  sprintf(data_send,"Hello, I am a little test data to send.");
  fprintf(stderr,"===[CLIENT]=== Send data\n");
  TRYFAIL(gras_trp_chunk_send(sock,data_send, sizeof(data_send)));
  
  fprintf(stderr,"===[CLIENT]=== Exiting successfully\n");
  gras_socket_close(sock);
   
  gras_exit();
  return 0;
}
