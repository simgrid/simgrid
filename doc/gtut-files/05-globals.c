#include <stdio.h>
#include <gras.h>

typedef struct {
   int killed;
} server_data_t;
   

int server_kill_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);
  server_data_t *globals=(server_data_t*)gras_userdata_get();
   
  fprintf(stderr,"Argh, killed by %s:%d! Bye folks...\n",
   	  gras_socket_peer_name(client), gras_socket_peer_port(client));
  
  globals->killed = 1;
   
  return 0;
} /* end_of_kill_callback */

int server_hello_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);

  fprintf(stderr,"Cool, we received the message from %s:%d.\n",
   	  gras_socket_peer_name(client), gras_socket_peer_port(client));
  
  return 0;
} /* end_of_hello_callback */

int server(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  server_data_t *globals;
  
  gras_init(&argc,argv);

  globals=gras_userdata_new(server_data_t);
  globals->killed=0;

  gras_msgtype_declare("hello", NULL);
  gras_msgtype_declare("kill", NULL);
  mysock = gras_socket_server(atoi(argv[1]));
   
  gras_cb_register("hello",&server_hello_cb);   
  gras_cb_register("kill",&server_kill_cb);

  while (!globals->killed) {
     gras_msg_handle(-1); /* blocking */
  }
    
  gras_exit();
  return 0;
}

int client(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  gras_socket_t toserver; /* socket used to write to the server */

  gras_init(&argc,argv);

  gras_msgtype_declare("hello", NULL);
  gras_msgtype_declare("kill", NULL);
  mysock = gras_socket_server_range(1024, 10000, 0, 0);
  
  fprintf(stderr,"Client ready; listening on %d\n", gras_socket_my_port(mysock));
  
  gras_os_sleep(1.5); /* sleep 1 second and half */
  toserver = gras_socket_client(argv[1], atoi(argv[2]));
  
  gras_msg_send(toserver,"hello", NULL);
  fprintf(stderr,"we sent the data to the server on %s. Let's do it again for fun\n", gras_socket_peer_name(toserver));
  gras_msg_send(toserver,"hello", NULL);
   
  fprintf(stderr,"Ok. Enough. Have a rest, and then kill the server\n");
  gras_os_sleep(5); /* sleep 1 second and half */
  gras_msg_send(toserver,"kill", NULL);

  gras_exit();
  return 0;
}
