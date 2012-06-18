/* numerous_rpc -- ensures that no race condition occurs when there is a    */
/*                 huge amount of very small messages                       */

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.               */
/* Thanks to Soumeya Leila Hernane for reporting an issue around this       */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

#define RPC_AMOUNT 50000 /* amount of RPC calls to do in a raw */

XBT_LOG_NEW_DEFAULT_CATEGORY(numerous_rpc,"logs attached to the RPC crash test");

int give_hostname(gras_msg_cb_ctx_t ctx, void *payload);
int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

/*************************************/

/* callback used for the test */
int give_hostname(gras_msg_cb_ctx_t ctx, void *payload) {            /* rpc, return hostname */
  const char* myself = gras_os_myname();

  gras_msg_rpcreturn(-1,ctx,&myself);

  gras_socket_close(gras_msg_cb_ctx_from(ctx));
  return 0;
}

/*************************************/
int server(int argc, char *argv[]) {
  xbt_socket_t mysock;
  int i;

  gras_init(&argc, argv);
  gras_msgtype_declare_rpc("give_hostname",xbt_datadesc_by_name("xbt_string_t"),xbt_datadesc_by_name("xbt_string_t"));
  gras_cb_register("give_hostname", &give_hostname);

  mysock = gras_socket_server(2222);

  for (i=0;i<RPC_AMOUNT;i++)
    gras_msg_handle(-1);

  gras_socket_close(mysock);
  gras_exit();
  return 0;

}

/* ****************** Code of the client ****************************** */
int client(int argc, char *argv[]) {
  long int i;
  xbt_socket_t server_sock,client_sock;
  xbt_string_t myself = xbt_strdup(gras_os_myname());
  xbt_string_t hostname = xbt_malloc(60);

  gras_init(&argc, argv);
  gras_msgtype_declare_rpc("give_hostname",xbt_datadesc_by_name("xbt_string_t"),xbt_datadesc_by_name("xbt_string_t"));

  client_sock = gras_socket_server_range(1024, 10000, 0, 0);

  for(i=0;i<RPC_AMOUNT;i++) {
    server_sock = gras_socket_client(argv[1], 2222);
    //if (i%1000==0)
      XBT_INFO("iteration %ld",i);
    gras_msg_rpccall(server_sock,-1,"give_hostname",&myself,&hostname);
    gras_socket_close(server_sock) ;
  }

  gras_socket_close(client_sock);

  xbt_free(hostname);
  xbt_free(myself);
  gras_exit();

  return 0;
}








