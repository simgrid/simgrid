#include "ping.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(Ping,"Messages specific to this example");

/* Global private data */
typedef struct {
  gras_socket_t sock;
  int endcondition;
} server_data_t;


/* register messages which may be sent (common to client and server) */
void ping_register_messages(void) {
  gras_msgtype_declare("ping", gras_datadesc_by_name("int"));
  gras_msgtype_declare("pong", gras_datadesc_by_name("int"));
  INFO0("Messages registered");
}

static int server_cb_ping_handler(gras_msg_cb_ctx_t ctx, void* payload) {
			     
  xbt_ex_t e;
  /* 1. Get the payload into the msg variable, and retrieve my caller */
  int msg=*(int*)payload;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 2. Retrieve the server's state (globals) */

  server_data_t *globals=(server_data_t*)gras_userdata_get();
  globals->endcondition = 0;
   
  /* 3. Log which client connected */
  INFO3(">>>>>>>> Got message PING(%d) from %s:%d <<<<<<<<", 
	msg, 
 	gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor));
  
  /* 4. Change the value of the msg variable */
  msg = 4321;
  /* 5. Send it back as payload of a pong message to the expeditor */
  TRY {
    gras_msg_send(expeditor, "pong", &msg);

  /* 6. Deal with errors: add some details to the exception */
  } CATCH(e) {
    gras_socket_close(globals->sock);
    RETHROW0("Unable answer with PONG: %s");
  }

  INFO0(">>>>>>>> Answered with PONG(4321) <<<<<<<<");
   
  /* 7. Set the endcondition boolean to true (and make sure the server stops after receiving it). */
  globals->endcondition = 1;
  
  /* 8. Make sure we don't leak sockets */
  gras_socket_close(expeditor);
   
  /* 9. Tell GRAS that we consummed this message */
  return 0;
} /* end_of_server_cb_ping_handler */

int server (int argc,char *argv[]) {
  server_data_t *globals;

  int port = 4000;
  
  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc,argv);
  globals=gras_userdata_new(server_data_t);
   
  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2) {
    port=atoi(argv[1]);
  }

  INFO1("Launch server (port=%d)", port);

  /* 3. Create my master socket */
  globals->sock = gras_socket_server(port);

  /* 4. Register the known messages. This function is called twice here, but it's because
        this file also acts as regression test, no need to do so yourself of course. */
  ping_register_messages();
  ping_register_messages(); /* just to make sure it works ;) */
   
  /* 5. Register my callback */
  gras_cb_register("ping",&server_cb_ping_handler);

  INFO1(">>>>>>>> Listening on port %d <<<<<<<<", gras_socket_my_port(globals->sock));
  globals->endcondition=0;

  /* 6. Wait up to 10 minutes for an incomming message to handle */
  gras_msg_handle(60.0);
   
  /* 7. Housekeeping */
  if (!globals->endcondition)
     WARN0("An error occured, the endcondition was not set by the callback");
  
  /* 8. Free the allocated resources, and shut GRAS down */
  gras_socket_close(globals->sock);
  free(globals);
  gras_exit();
   
  INFO0("Done.");
  return 0;
} /* end_of_server */


int client(int argc,char *argv[]) {
  xbt_ex_t e; 
  gras_socket_t toserver=NULL; /* peer */

  gras_socket_t from;
  int ping, pong;

  const char *host = "127.0.0.1";
        int   port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);
   
  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 3) {
    host=argv[1];
    port=atoi(argv[2]);
  } 

  INFO2("Launch client (server on %s:%d)",host,port);
   
  /* 3. Wait for the server startup */
  gras_os_sleep(1);
   
  /* 4. Create a socket to speak to the server */
  TRY {
    toserver=gras_socket_client(host,port);
  } CATCH(e) {
    RETHROW0("Unable to connect to the server: %s");
  }
  INFO2("Connected to %s:%d.",host,port);    


  /* 5. Register the messages. 
        See, it doesn't have to be done completely at the beginning, only before use */
  ping_register_messages();

  /* 6. Keep the user informed of what's going on */
  INFO2(">>>>>>>> Connected to server which is on %s:%d <<<<<<<<", 
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 7. Prepare and send the ping message to the server */
  ping = 1234;
  TRY {
    gras_msg_send(toserver, "ping", &ping);
  } CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Failed to send PING to server: %s");
  }
  INFO3(">>>>>>>> Message PING(%d) sent to %s:%d <<<<<<<<",
	ping,
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 8. Wait for the answer from the server, and deal with issues */
  TRY {
    gras_msg_wait(6000,"pong", &from,&pong);
  } CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Why can't I get my PONG message like everyone else: %s");
  }

  /* 9. Keep the user informed of what's going on, again */
  INFO3(">>>>>>>> Got PONG(%d) from %s:%d <<<<<<<<", 
	pong,
	gras_socket_peer_name(from),gras_socket_peer_port(from));

  /* 10. Free the allocated resources, and shut GRAS down */
  gras_socket_close(toserver);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_client */
