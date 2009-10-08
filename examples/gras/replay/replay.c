/* Copyright (c) 2009 Da SimGrid Team.  All rights reserved.                */

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
#include "workload.h"
#include "gras.h"
#include "amok/peermanagement.h"
#include <stdio.h>

int master(int argc,char *argv[]);
int worker(int argc,char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(replay, "Messages specific to this example");

static void declare_msg() {
  gras_datadesc_type_t command_assignment_type;


}

int master(int argc,char *argv[]) {
  gras_init(&argc,argv);
  amok_pm_init();

  xbt_assert0(argc==3,"usage: replay_master tracefile port");
  gras_socket_server(atoi(argv[2])); /* open my master socket, even if I don't use it */
  xbt_dynar_t peers = amok_pm_group_new("replay");            /* group of slaves */
  xbt_dynar_t cmds = xbt_workload_parse_file(argv[1]);
  xbt_workload_sort_who_date(cmds);
  unsigned int cursor;
  xbt_workload_elm_t cmd;

  xbt_dynar_foreach(cmds,cursor,cmd) {
    char *str = xbt_workload_elm_to_string(cmd);
    INFO1("%s",str);
    free(str);
  }

  /* friends, we're ready. Come and play */
  INFO0("Wait for peers for 5 sec");
  gras_msg_handleall(5);
  INFO1("Got %ld pals", xbt_dynar_length(peers));


  /* Done, exiting */
  xbt_dynar_free(&cmds);
  gras_exit();
  return 0;
}

int worker(int argc,char *argv[]) {
  gras_init(&argc,argv);
  amok_pm_init();

  gras_exit();
  return 0;
}
