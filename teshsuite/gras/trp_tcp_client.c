/* $Id$ */

/* trp_tcp_client: Client of a test case for the tcp transport.             */

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

int main(int argc,char *argv[]) {
  gras_socket_t sock;
  char data_send[256];
  char data_recv[256];

  memset(data_send, 0, sizeof(data_send));
  memset(data_recv, 0, sizeof(data_recv));

  gras_init(&argc,argv);

  fprintf(stderr,"===[CLIENT]=== Contact the server\n");
  sock = gras_socket_client(NULL,55555);

  sprintf(data_send,"Hello, I am a little test data to send.");
  fprintf(stderr,"===[CLIENT]=== Send data\n");
  gras_trp_send(sock,data_send, sizeof(data_send),1);
  gras_trp_flush(sock);
  fprintf(stderr,"===[CLIENT]=== Waiting for the ACK\n");
  gras_trp_recv(sock,data_recv, sizeof(data_recv));
  
  if (strcmp(data_send, data_recv)) {
    fprintf(stderr, "===[CLIENT]=== String sent != string received\n");
    xbt_abort();
  } 
  fprintf(stderr,"===[CLIENT]=== Got a valid ACK\n");
  
  fprintf(stderr,"===[CLIENT]=== Exiting successfully\n");
  gras_socket_close(sock);
   
  gras_exit();
  return 0;
}
