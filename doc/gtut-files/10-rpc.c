/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"My little example");

typedef struct {
   int done;
} server_data_t;
int server_done_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  server_data_t *globals=(server_data_t*)gras_userdata_get();
  globals->done = 1;
  INFO0("Server done");
   
  return 0;
} /* end_of_done_callback */

void message_declaration(void) {
  gras_msgtype_declare_rpc("convert a2i", gras_datadesc_by_name("string"), gras_datadesc_by_name("long"));
  gras_msgtype_declare_rpc("convert i2a", gras_datadesc_by_name("long"), gras_datadesc_by_name("string"));
 
  /* the other message allowing the client to stop the server after use */
  gras_msgtype_declare("done", NULL); 
}

int server_convert_i2a_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  long data = *(long*)payload;
  char *result;
  char *p;

  INFO1("Convert %ld to string",data);  
  result = bprintf("%ld", data);   
  INFO2("%ld converted to string: %s",data,result);
   
  gras_msg_rpcreturn(60,ctx,&result);
  free(result);
  return 0;
} /* end_of_convert_callback */

int server_convert_a2i_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  char *string = *(char**)payload;
  long result;
  char *p;

  INFO1("Convert %s to long",string);
  result = strtol(string, &p, 10);
   
  if (*p != '\0')
     THROW2(arg_error,0,"Error while converting %s: this does not seem to be a valid number (problem at '%s')",string,p);
	      
  gras_msg_rpcreturn(60,ctx,&result);
  return 0;
} /* end_of_convert_callback */


int server(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  server_data_t *globals;
  
  gras_init(&argc,argv);

  globals=gras_userdata_new(server_data_t*);
  globals->done=0;

  message_declaration();
  mysock = gras_socket_server(atoi(argv[1]));
   
  gras_cb_register("convert a2i",&server_convert_a2i_cb);
  gras_cb_register("convert i2a",&server_convert_i2a_cb);
  gras_cb_register("done",&server_done_cb);

  while (!globals->done) {
     gras_msg_handle(-1); /* blocking */
  }
    
  gras_exit();
  return 0;
}

int client(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  gras_socket_t toserver; /* socket used to write to the server */

  gras_init(&argc,argv);

  message_declaration();
  mysock = gras_socket_server_range(1024, 10000, 0, 0);
  
  VERB1("Client ready; listening on %d", gras_socket_my_port(mysock));
  
  gras_os_sleep(1.5); /* sleep 1 second and half */
  toserver = gras_socket_client(argv[1], atoi(argv[2]));
  
  long long_to_convert=4321;
  char *string_result;
  INFO1("Ask to convert %ld", long_to_convert);
  gras_msg_rpccall(toserver, 60, "convert i2a", &long_to_convert, &string_result);
  INFO2("The server says that %ld is equal to \"%s\".", long_to_convert, string_result);
  free(string_result);
   
  char *string_to_convert="1234";
  long long_result;
  INFO1("Ask to convert %s", string_to_convert);
  gras_msg_rpccall(toserver, 60, "convert a2i", &string_to_convert, &long_result);
  INFO2("The server says that \"%s\" is equal to %d.", string_to_convert, long_result);
   
  xbt_ex_t e;
  string_to_convert = "azerty";
  TRY {
     gras_msg_rpccall(toserver, 60, "convert a2i", &string_to_convert, &long_result);
  } CATCH(e) {
     INFO1("The server refuses to convert %s. Here is the received exception:",string_to_convert);
     xbt_ex_display(&e);
     xbt_ex_free(e);
     INFO0("Again, previous exception was excepted");
  }    	
   
  gras_msg_send(toserver,"done", NULL);
  INFO0("Stopped the server");
   
  gras_exit();
  return 0;
}
