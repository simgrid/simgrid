/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example replays a trace as produced by examples/simdag/dax, or other means
 * This is mainly interesting when run on real platforms, to validate the results
 * given in the simulator when running SimDag.
 */

#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dynar.h"
#include "xbt/synchro.h"
#include "workload.h"
#include "gras.h"
#include "amok/peermanagement.h"
#include <stdio.h>

#include "simix/simix.h"

int master(int argc,char *argv[]);
int worker(int argc,char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(replay, "Messages specific to this example");

static void declare_msg() {
  amok_pm_init();
  xbt_workload_declare_datadesc();
  gras_msgtype_declare("go",NULL);
  gras_msgtype_declare("commands",
      gras_datadesc_dynar(
          gras_datadesc_by_name("xbt_workload_elm_t"),xbt_workload_elm_free_voidp));
  gras_msgtype_declare("chunk",gras_datadesc_by_name("xbt_workload_data_chunk_t"));
}

int master(int argc,char *argv[]) {

  gras_init(&argc,argv);
  declare_msg();


  xbt_assert0(argc==3,"usage: replay_master tracefile port");
  gras_socket_server(atoi(argv[2])); /* open my master socket, even if I don't use it */
  xbt_dynar_t peers = amok_pm_group_new("replay");            /* group of slaves */
  xbt_peer_t peer;
  xbt_dynar_t cmds = xbt_workload_parse_file(argv[1]);
  xbt_workload_sort_who_date(cmds);
  unsigned int cursor;
  xbt_workload_elm_t cmd;

  xbt_ex_t e;
  xbt_dict_cursor_t dict_cursor;

  xbt_dict_t pals_int=xbt_dict_new();
  xbt_dynar_foreach(cmds,cursor,cmd) {
    int *p = xbt_dict_get_or_null(pals_int,cmd->who);
    if (!p) {
      p=(int*)0xBEAF;
      xbt_dict_set(pals_int,cmd->who,&p,NULL);
    }
  }

  /* friends, we're ready. Come and play */
  INFO1("Wait for peers for a while. I need %d peers",xbt_dict_size(pals_int));
  while (xbt_dynar_length(peers)<xbt_dict_size(pals_int)) {
    TRY {
      gras_msg_handle(20);
    } CATCH(e) {
      xbt_dynar_foreach(peers,cursor,peer){
        xbt_dict_remove(pals_int,peer->name);
      }
      char *peer_name;
      void *data;
      xbt_dict_foreach(pals_int,dict_cursor,peer_name,data) {
        INFO1("Peer %s didn't showed up",peer_name);
      }
      RETHROW;
    }
  }
  INFO1("Got my %ld peers", xbt_dynar_length(peers));
  xbt_dict_free(&pals_int);

  /* Check who came */
  xbt_dict_t pals = xbt_dict_new();
  gras_socket_t pal;
  xbt_dynar_foreach(peers,cursor, peer) {
    //INFO1("%s is here",peer->name);
    gras_socket_t sock = gras_socket_client(peer->name,peer->port);
    xbt_dict_set(pals,peer->name,sock,NULL);
  }
  /* check that we have a dude for every element of the trace */
  xbt_dynar_foreach(cmds,cursor,cmd) {
    pal=xbt_dict_get_or_null(pals,cmd->who);
    if (!pal) {
      CRITICAL1("Process %s didn't came! Abording!",cmd->who);
      amok_pm_group_shutdown("replay");
      xbt_dynar_free(&cmds);
      gras_exit();
      abort();
    }
  }
  /* Send the commands to every pal */
  char *pal_name;
  xbt_dict_foreach(pals,dict_cursor,pal_name,pal) {
    gras_msg_send(pal,"commands",&cmds);
  }
  INFO0("Sent commands to every processes. Let them start now");
  xbt_dict_cursor_t dict_it;
  xbt_dict_foreach(pals,dict_it,pal_name,pal) {
    gras_msg_send(pal,"go",NULL);
  }
  INFO0("They should be started by now. Wait for their completion.");

  for (cursor=0;cursor<xbt_dynar_length(peers);cursor++) {
    gras_msg_wait(-1,"go",NULL,NULL);
  }

  /* Done, exiting */
  //amok_pm_group_shutdown("replay");
  xbt_dynar_free(&cmds);
  gras_exit();
  return 0;
}

typedef struct {
  xbt_dynar_t commands;
  xbt_dict_t peers;
  gras_socket_t mysock;
} s_worker_data_t,*worker_data_t;


static gras_socket_t get_peer_sock(char*peer) {
  worker_data_t g = gras_userdata_get();
  gras_socket_t peer_sock = xbt_dict_get_or_null(g->peers,peer);
  if (!peer_sock) {
    INFO1("Create a socket to %s",peer);
    peer_sock = gras_socket_client(peer,4000);
    xbt_dict_set(g->peers,peer,peer_sock,NULL);//gras_socket_close_voidp);
  }
  return peer_sock;
}
static int worker_commands_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  worker_data_t g = gras_userdata_get();
  g->commands = *(xbt_dynar_t*)payload;
  return 0;
}
static void do_command(int rank, void*c) {
  xbt_ex_t e;
  xbt_workload_elm_t cmd = *(xbt_workload_elm_t*)c;
  xbt_workload_data_chunk_t chunk;

  if (cmd->action == XBT_WORKLOAD_SEND) {
    gras_socket_t sock = get_peer_sock(cmd->str_arg);
    chunk = xbt_workload_data_chunk_new((int)(cmd->d_arg));
    INFO4("Send %.f bytes to %s %s:%d",cmd->d_arg,cmd->str_arg,
        gras_socket_peer_name(sock),gras_socket_peer_port(sock));
    gras_msg_send_(sock,gras_msgtype_by_name("chunk"),&chunk);
    INFO2("Done sending %.f bytes to %s",cmd->d_arg,cmd->str_arg);

  } else if (cmd->action == XBT_WORKLOAD_RECV) {
    INFO2("Recv %.f bytes from %s",cmd->d_arg,cmd->str_arg);
    TRY {
      gras_msg_wait(1000000,"chunk",NULL,&chunk);
    } CATCH(e) {
      SIMIX_display_process_status();
      RETHROW2("Exception while waiting for %f bytes from %s: %s",
            cmd->d_arg,cmd->str_arg);
    }
    xbt_workload_data_chunk_free(chunk);
    INFO2("Done receiving %.f bytes from %s",cmd->d_arg,cmd->str_arg);

  } else {
    xbt_die(bprintf("unknown command: %s",xbt_workload_elm_to_string(cmd)));
  }
}
int worker(int argc,char *argv[]) {
  xbt_ex_t e;
  worker_data_t globals;
  gras_init(&argc,argv);
  declare_msg();
  globals = gras_userdata_new(s_worker_data_t);
  /* Create the connexions */
  globals->mysock = gras_socket_server(4000); /* FIXME: shouldn't be hardcoded */
  gras_socket_t master=NULL;
  int connected=0;

  gras_cb_register("commands", worker_commands_cb);
  globals->peers=xbt_dict_new();

  if (gras_if_RL())
    INFO2("Sensor %s starting. Connecting to master on %s", gras_os_myname(), argv[1]);
  while (!connected) {
    xbt_ex_t e;
    TRY {
      master = gras_socket_client_from_string(argv[1]);
      connected = 1;
    }
    CATCH(e) {
      if (e.category != system_error)
        RETHROW;
      xbt_ex_free(e);
      INFO0("Failed to connect. Retry in 0.5 second");
      gras_os_sleep(0.5);
    }
  }
  /* Join and run the group */
  amok_pm_group_join(master, "replay", -1);
  gras_msg_handle(60); // command message
  gras_msg_wait(60,"go",NULL,NULL);
  {
    worker_data_t g = gras_userdata_get();
    unsigned int cursor;
    xbt_workload_elm_t cmd;
    const char *myname=gras_os_myname();
    xbt_dynar_t cmd_to_go = xbt_dynar_new(sizeof(xbt_workload_elm_t),NULL);

    xbt_dynar_foreach(g->commands,cursor,cmd) {
      if (!strcmp(cmd->who,myname)) {
        char *c = xbt_workload_elm_to_string(cmd);
  //      INFO1("TODO: %s",c);
        free(c);

        switch (cmd->action) {
        case XBT_WORKLOAD_COMPUTE:
          /* If any communication were queued, do them in parallel */
          if (xbt_dynar_length(cmd_to_go)){
            TRY {
              xbt_dynar_dopar(cmd_to_go,do_command);
              xbt_dynar_reset(cmd_to_go);
            } CATCH(e) {
              SIMIX_display_process_status();
            }
            INFO0("Communications all done");
            xbt_dynar_reset(cmd_to_go);
          }
          INFO1("Compute %.f flops",cmd->d_arg);
          gras_cpu_burn(cmd->d_arg);
          INFO1("Done computing %.f flops",cmd->d_arg);
          break;
        case XBT_WORKLOAD_SEND:
          /* Create the socket from main thread since it seems to fails when done from dopar thread */
          get_peer_sock(cmd->str_arg);
        case XBT_WORKLOAD_RECV:
          /* queue communications for later realization in parallel */
          xbt_dynar_push(cmd_to_go,&cmd);
          break;
        }
      }
    }
    /* do in parallel any communication still queued */
    INFO1("Do %ld pending communications after end of TODO list",xbt_dynar_length(cmd_to_go));
    if (xbt_dynar_length(cmd_to_go)){
      xbt_dynar_dopar(cmd_to_go,do_command);
      xbt_dynar_reset(cmd_to_go);
    }
  }

  gras_msg_send(master,"go",NULL);
//  amok_pm_group_leave(master, "replay");

  gras_socket_close(globals->mysock);
  xbt_dynar_free(&(globals->commands));
  xbt_dict_free(&(globals->peers));
  free(globals);

  gras_exit();
  return 0;
}
