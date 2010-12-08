/* Example of traces replay without context switches, running MPI actions */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "replay.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dynar.h"
#include "simix/smurf_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(mpi_replay,"MPI replayer");


/* Helper function */
static double parse_double(const char *string) {
  double value;
  char *endptr;

  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROW1(unknown_error, 0, "%s is not a double", string);
  return value;
}
static int get_rank (const char *process_name){
  return atoi(&(process_name[1]));
}

const char *state_name[] = {
    "pump",
    "compute0", "compute1", "compute2",
    "send0","send1","send2","send3",
    "irecv0",
    "recv0",
    "wait0",
    "init0", "init1"
};
typedef enum {
  e_mpi_pump_evt_trace=0,
  e_mpi_compute0,e_mpi_compute1,e_mpi_compute2,
  e_mpi_send0,e_mpi_send1,e_mpi_send2,e_mpi_send3,
  e_mpi_irecv0,
  e_mpi_recv0,
  e_mpi_wait0,
  e_mpi_init0,e_mpi_init1
} e_mpi_replay_state_t;

typedef struct {
  /* Myself */
  char *procname;
  smx_host_t myhost;
  e_mpi_replay_state_t state;
  /* Parsing logic */
  replay_trace_reader_t reader;
  const char * const*evt;
  /* simix interface */
  smx_action_t act;

  xbt_dynar_t isends; /* of msg_comm_t, cleaned up automatically on send event */
} s_mpi_replay_t, *mpi_replay_t;

static void *mpi_replay_init(int argc, char *argv[]) {

  mpi_replay_t res = xbt_new0(s_mpi_replay_t,1);
  res->procname = xbt_strdup(argv[0]);
  res->state = e_mpi_pump_evt_trace;

  res->reader = replay_trace_reader_new(argv[1]);
  return res;
}

static void mpi_replay_run(void*data,void *syscall_res) {
  mpi_replay_t g = (mpi_replay_t)data; /* my globals */

  new_event:
  INFO2("mpi_replay_run, state=%s (%d)",state_name[g->state],g->state);

  switch(g->state){

  case e_mpi_pump_evt_trace: { /* nothing to do, parse next event and call function again */

    g->evt = replay_trace_reader_get(g->reader);
    if (strcmp(g->procname, g->evt[0])) {
      WARN1("Ignore trace element not for me at %s",
          replay_trace_reader_position(g->reader));
      goto new_event;
    }

    if (!strcmp(g->evt[1],"send")) {
      g->state = e_mpi_send0;
      goto new_event;
    } else if (!strcmp(g->evt[1],"recv")) {
      g->state = e_mpi_recv0;
      goto new_event;
    } else if (!strcmp(g->evt[1],"irecv")||!strcmp(g->evt[1],"Irecv")) {
      g->state = e_mpi_irecv0;
      goto new_event;
    } else if (!strcmp(g->evt[1],"wait")) {
      g->state = e_mpi_wait0;
      goto new_event;
    } else if (!strcmp(g->evt[1],"compute")) {
      g->state = e_mpi_compute0;
      goto new_event;
    } else if (!strcmp(g->evt[1],"init")) {
      g->state = e_mpi_init0;
      goto new_event;
    } else {
      WARN2("Ignoring unrecognized trace element at %s: %s",
          replay_trace_reader_position(g->reader),g->evt[1]);
      goto new_event;
    }
  } THROW_IMPOSSIBLE;

  /* *** Send *** */
  case e_mpi_send0: {
    char to[250];
    sprintf(to, "%s_%s", g->procname, g->evt[2]);

    DEBUG2("Entering Send at %s (size: %lg)",
        replay_trace_reader_position(g->reader), parse_double(g->evt[3]));
    g->state = e_mpi_send1;
    SIMIX_req_rdv_create(to);
  } THROW_IMPOSSIBLE;

  case e_mpi_send1:
    g->state = e_mpi_send2;
    SIMIX_req_comm_isend(syscall_res, parse_double(g->evt[3]),-1,
        NULL,0,//void *src_buff, size_t src_buff_size,
        NULL,NULL);//int (*match_fun)(void *, void *), void *data)
    THROW_IMPOSSIBLE;
  case e_mpi_send2:
    if (parse_double(g->evt[3])<65536) {
      xbt_dynar_push(g->isends,&syscall_res);
      g->state = e_mpi_pump_evt_trace;
      goto new_event;
    }
    g->act = syscall_res;
    g->state=e_mpi_send3;
    SIMIX_req_comm_wait(g->act,-1);
  case e_mpi_send3:
    g->state=e_mpi_pump_evt_trace;
    SIMIX_req_comm_destroy(g->act);

  /* *** Computation *** */
  case e_mpi_compute0:
    g->state = e_mpi_compute1;
    SIMIX_req_host_execute(replay_trace_reader_position(g->reader),
        g->myhost,parse_double(g->evt[2]));
    THROW_IMPOSSIBLE;
  case e_mpi_compute1:
    g->act = syscall_res;
    g->state = e_mpi_compute2;
    SIMIX_req_host_execution_wait(g->act);
    THROW_IMPOSSIBLE;
  case e_mpi_compute2:
    g->state = e_mpi_pump_evt_trace;
    SIMIX_req_host_execution_destroy(g->act);
    THROW_IMPOSSIBLE;

  case e_mpi_irecv0: xbt_die("irecv0 unimplemented");
  case e_mpi_recv0: xbt_die("recv0 unimplemented");
  case e_mpi_wait0: xbt_die("wait0 unimplemented");
  case e_mpi_init0:
    g->state = e_mpi_init1;
    SIMIX_req_process_get_host(SIMIX_process_self());
    THROW_IMPOSSIBLE;
  case e_mpi_init1:
    g->myhost = syscall_res;
    g->state = e_mpi_pump_evt_trace;
    goto new_event;

  }
  THROW_IMPOSSIBLE;
}
static void mpi_replay_fini(void *data) {
  mpi_replay_t g = (mpi_replay_t)data;
  replay_trace_reader_free(&g->reader);
  free(data);
}

int main(int argc, char *argv[]) {
  SG_replay_init(&argc,argv);
  if (argc<3) {
    fprintf(stderr,"USAGE: replay platform_file deployment_file\n");
    exit(1);
  }
  SG_replay_set_functions(mpi_replay_init,mpi_replay_run,mpi_replay_fini);
  SG_replay(argv[1],argv[2]);

  return 0;
}
