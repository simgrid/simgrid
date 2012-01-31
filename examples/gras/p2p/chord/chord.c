/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "xbt/sysdep.h"
#include "gras.h"

static int closest_preceding_node(int id);
static void check_predecessor(void);

XBT_LOG_NEW_DEFAULT_CATEGORY(chord, "Messages specific to this example");

typedef enum msg_typus {
  PING,
  PONG,
  GET_PRE,
  REP_PRE,
  GET_SUC,
  REP_SUC,
  STD,
} msg_typus;

/*XBT_DEFINE_TYPE(s_pbio,
  struct s_pbio{
    msg_typus type;
    int dest;
    char msg[1024];
  };
);
typedef struct s_pbio pbio_t;*/

/*XBT_DEFINE_TYPE(s_ping,*/
struct s_ping {
  int id;
};
/*);*/
typedef struct s_ping ping_t;

/*XBT_DEFINE_TYPE(s_pong,*/
struct s_pong {
  int id;
  int failed;
};
/*);*/
typedef struct s_pong pong_t;

XBT_DEFINE_TYPE(s_notify, struct s_notify {
                 int id; char host[1024]; int port;};);

typedef struct s_notify notify_t;

XBT_DEFINE_TYPE(s_get_suc, struct s_get_suc {
                 int id;};);

typedef struct s_get_suc get_suc_t;

XBT_DEFINE_TYPE(s_rep_suc, struct s_rep_suc {
                 int id; char host[1024]; int port;};);

typedef struct s_rep_suc rep_suc_t;

typedef struct finger_elem {
  int id;
  char host[1024];
  int port;
} finger_elem;



static void register_messages()
{
/*  gras_msgtype_declare("chord",xbt_datadesc_by_symbol(s_pbio));*/
  gras_msgtype_declare("chord_get_suc",
                       xbt_datadesc_by_symbol(s_get_suc));
  gras_msgtype_declare("chord_rep_suc",
                       xbt_datadesc_by_symbol(s_rep_suc));
  gras_msgtype_declare("chord_notify", xbt_datadesc_by_symbol(s_notify));
}

/* Global private data */
typedef struct {
  xbt_socket_t sock;           /* server socket on which I'm listening */
  int id;                       /* my id number */
  char host[1024];              /* my host name */
  int port;                     /* port on which I'm listening FIXME */
  int fingers;                  /* how many fingers */
  finger_elem *finger;          /* finger table */
  int next_to_fix;              /* next in the finger list to be checked */
  int pre_id;                   /* predecessor id */
  char pre_host[1024];          /* predecessor host */
  int pre_port;                 /* predecessor port */
} node_data_t;


int node(int argc, char **argv);

/*static int node_cb_chord_handler(xbt_socket_t expeditor,void *payload_data){
  xbt_ex_t e;
  pbio_t pbio_i=*(pbio_t*)payload_data;

  node_data_t *globals=(node_data_t*)gras_userdata_get();

  XBT_INFO(">>> %d : received %d message from %s to %d <<<",globals->id,pbio_i.type,xbt_socket_peer_name(expeditor),pbio_i.dest);



}*/

static int node_cb_get_suc_handler(gras_msg_cb_ctx_t ctx,
                                   void *payload_data)
{
  xbt_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  get_suc_t incoming = *(get_suc_t *) payload_data;
  rep_suc_t outgoing;
  node_data_t *globals = (node_data_t *) gras_userdata_get();
  XBT_INFO("Received a get_successor message from %s for %d",
        xbt_socket_peer_name(expeditor), incoming.id);
  if ((globals->id == globals->finger[0].id) ||
      (incoming.id > globals->id
       && incoming.id <= globals->finger[0].id)) {
    outgoing.id = globals->finger[0].id;
    snprintf(outgoing.host, 1024, globals->finger[0].host);
    outgoing.port = globals->finger[0].port;
    XBT_INFO("My successor is his successor!");
  } else {
    xbt_socket_t temp_sock;
    int contact = closest_preceding_node(incoming.id);
    if (contact == -1) {
      outgoing.id = globals->finger[0].id;
      snprintf(outgoing.host, 1024, globals->finger[0].host);
      outgoing.port = globals->finger[0].port;
      XBT_INFO("My successor is his successor!");
    } else {
      get_suc_t asking;
      asking.id = incoming.id;
      TRY {
        temp_sock = gras_socket_client(globals->finger[contact].host,
                                       globals->finger[contact].port);
      }
      CATCH_ANONYMOUS {
        RETHROWF("Unable to connect!: %s");
      }
      TRY {
        gras_msg_send(temp_sock, "chord_get_suc", &asking);
      }
      CATCH_ANONYMOUS {
        RETHROWF("Unable to ask!: %s");
      }
      gras_msg_wait(10., "chord_rep_suc", &temp_sock, &outgoing);
    }
  }

  TRY {
    gras_msg_send(expeditor, "chord_rep_suc", &outgoing);
    XBT_INFO("Successor information sent!");
  }
  CATCH_ANONYMOUS {
    RETHROWF("%s:Timeout sending successor information to %s: %s",
             globals->host, xbt_socket_peer_name(expeditor));
  }
  gras_socket_close(expeditor);
  return 0;
}

static int closest_preceding_node(int id)
{
  node_data_t *globals = (node_data_t *) gras_userdata_get();
  int i;
  for (i = globals->fingers - 1; i >= 0; i--) {
    if (globals->finger[i].id > globals->id && globals->finger[i].id < id) {
      return (i);
    }
  }

  return i;
}

static int node_cb_notify_handler(gras_msg_cb_ctx_t ctx,
                                  void *payload_data)
{
  xbt_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  /*xbt_ex_t e; */
  notify_t incoming = *(notify_t *) payload_data;
  node_data_t *globals = (node_data_t *) gras_userdata_get();
  XBT_INFO("Received a notifying message from %s as %d",
        xbt_socket_peer_name(expeditor), incoming.id);
  if (globals->pre_id == -1 ||
      (incoming.id > globals->pre_id && incoming.id < globals->id)) {
    globals->pre_id = incoming.id;
    snprintf(globals->pre_host, 1024, incoming.host);
    globals->pre_port = incoming.port;
    XBT_INFO("Set as my new predecessor!");
  }
  return 0;
}

static void fix_fingers()
{
  get_suc_t get_suc_msg;
  xbt_socket_t temp_sock = NULL;
  xbt_socket_t temp_sock2 = NULL;
  rep_suc_t rep_suc_msg;
  node_data_t *globals = (node_data_t *) gras_userdata_get();

  TRY {
    temp_sock = gras_socket_client(globals->host, globals->port);
  }
  CATCH_ANONYMOUS {
    RETHROWF("Unable to contact known host: %s");
  }

  get_suc_msg.id = globals->id;
  TRY {
    gras_msg_send(temp_sock, "chord_get_suc", &get_suc_msg);
  }
  CATCH_ANONYMOUS {
    gras_socket_close(temp_sock);
    RETHROWF("Unable to contact known host to get successor!: %s");
  }

  TRY {
    XBT_INFO("Waiting for reply!");
    gras_msg_wait(6000, "chord_rep_suc", &temp_sock2, &rep_suc_msg);
  }
  CATCH_ANONYMOUS {
    RETHROWF("%s: Error waiting for successor:%s", globals->host);
  }
  globals->finger[0].id = rep_suc_msg.id;
  snprintf(globals->finger[0].host, 1024, rep_suc_msg.host);
  globals->finger[0].port = rep_suc_msg.port;
  XBT_INFO("→ Finger %d fixed!", globals->next_to_fix);
  gras_socket_close(temp_sock);

  globals->next_to_fix = (++globals->next_to_fix == globals->fingers) ?
      0 : globals->next_to_fix;
}

static void check_predecessor()
{
  node_data_t *globals = (node_data_t *) gras_userdata_get();
  xbt_socket_t temp_sock;
  ping_t ping;
  pong_t pong;
  xbt_ex_t e;
  if (globals->pre_id == -1) {
    return;
  }
  TRY {
    temp_sock = gras_socket_client(globals->pre_host, globals->pre_port);
  }
  CATCH(e) {
    globals->pre_id = -1;
    globals->pre_host[0] = 0;
    globals->pre_port = 0;
    xbt_ex_free(e);
  }

  ping.id = 0;
  TRY {
    gras_msg_send(temp_sock, "chord_ping", &ping);
  }
  CATCH(e) {
    globals->pre_id = -1;
    globals->pre_host[0] = 0;
    globals->pre_port = 0;
    xbt_ex_free(e);
  }
  TRY {
    gras_msg_wait(60, "chord_pong", &temp_sock, &pong);
  }
  CATCH(e) {
    globals->pre_id = -1;
    globals->pre_host[0] = 0;
    globals->pre_port = 0;
    xbt_ex_free(e);
  }
  gras_socket_close(temp_sock);
}

int node(int argc, char **argv)
{
  node_data_t *globals = NULL;
  xbt_socket_t temp_sock = NULL;
  xbt_socket_t temp_sock2 = NULL;
  get_suc_t get_suc_msg;
  rep_suc_t rep_suc_msg;

  xbt_ex_t e;

  int create = 0;
  int other_port = -1;
  char *other_host;
  notify_t notify_msg;
  int l;

  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);

  gras_os_sleep((15 - gras_os_getpid()) * 20);

  globals = gras_userdata_new(node_data_t);

  globals->id = atoi(argv[1]);
  globals->port = atoi(argv[2]);
  globals->fingers = 0;
  globals->finger = NULL;
  globals->pre_id = -1;
  globals->pre_host[0] = 0;
  globals->pre_port = -1;

  snprintf(globals->host, 1024, gras_os_myname());

  if (argc == 3) {
    create = 1;
  } else {
    other_host = xbt_strdup(argv[3]);
    other_port = atoi(argv[4]);
  }

  globals->sock = gras_socket_server(globals->port);
  gras_os_sleep(1.0);

  register_messages();

  globals->finger = (finger_elem *) calloc(1, sizeof(finger_elem));
  XBT_INFO("Launching node %s:%d", globals->host, globals->port);
  if (create) {
    XBT_INFO("→Creating ring");
    globals->finger[0].id = globals->id;
    snprintf(globals->finger[0].host, 1024, globals->host);
    globals->finger[0].port = globals->port;
  } else {
    XBT_INFO("→Known node %s:%d", other_host, other_port);
    XBT_INFO("→Contacting to determine successor");
    TRY {
      temp_sock = gras_socket_client(other_host, other_port);
    }
    CATCH_ANONYMOUS {
      RETHROWF("Unable to contact known host: %s");
    }

    get_suc_msg.id = globals->id;
    TRY {
      gras_msg_send(temp_sock, "chord_get_suc", &get_suc_msg);
    }
    CATCH_ANONYMOUS {
      gras_socket_close(temp_sock);
      RETHROWF("Unable to contact known host to get successor!: %s");
    }

    TRY {
      XBT_INFO("Waiting for reply!");
      gras_msg_wait(10., "chord_rep_suc", &temp_sock2, &rep_suc_msg);
    }
    CATCH_ANONYMOUS {
      RETHROWF("%s: Error waiting for successor:%s", globals->host);
    }
    globals->finger[0].id = rep_suc_msg.id;
    snprintf(globals->finger[0].host, 1024, rep_suc_msg.host);
    globals->finger[0].port = rep_suc_msg.port;
    XBT_INFO("→Got successor : %d-%s:%d", globals->finger[0].id,
          globals->finger[0].host, globals->finger[0].port);
    gras_socket_close(temp_sock);
    TRY {
      temp_sock = gras_socket_client(globals->finger[0].host,
                                     globals->finger[0].port);
    }
    CATCH_ANONYMOUS {
      RETHROWF("Unable to contact successor: %s");
    }

    notify_msg.id = globals->id;
    snprintf(notify_msg.host, 1024, globals->host);
    notify_msg.port = globals->port;
    TRY {
      gras_msg_send(temp_sock, "chord_notify", &notify_msg);
    }
    CATCH_ANONYMOUS {
      RETHROWF("Unable to notify successor! %s");
    }
  }

  gras_cb_register("chord_get_suc", &node_cb_get_suc_handler);
  gras_cb_register("chord_notify", &node_cb_notify_handler);
  /*gras_cb_register("chord_ping",&node_cb_ping_handler); */
  /* gras_timer_repeat(600.,fix_fingers); */
  /*while(1){ */

  for (l = 0; l < 50; l++) {
    TRY {
      gras_msg_handle(6000000.0);
    }
    CATCH(e) {
      xbt_ex_free(e);
    }
  }
  /*} */

  gras_socket_close(globals->sock);
  free(globals);
  gras_exit();
  XBT_INFO("Done");
  return (0);
}
