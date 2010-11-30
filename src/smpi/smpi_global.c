/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>
#include <stdlib.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"
#include "mc/mc.h"

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories");

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi,
                                "Logging specific to SMPI (kernel)");

typedef struct s_smpi_process_data {
  int index;
  int* argc;
  char*** argv;
  xbt_fifo_t pending_sent;
  xbt_fifo_t pending_recv;
  xbt_os_timer_t timer;
  double simulated;
  MPI_Comm comm_self;
} s_smpi_process_data_t;

static smpi_process_data_t *process_data = NULL;
static int process_count = 0;

MPI_Comm MPI_COMM_WORLD = MPI_COMM_NULL;

void smpi_process_init(int *argc, char ***argv)
{
  int index;
  smpi_process_data_t data;
  smx_process_t proc;

  if(argc && argv) {
    proc = SIMIX_process_self();
    index = atoi((*argv)[1]);
    data = smpi_process_remote_data(index);
    SIMIX_process_set_data(proc, data);
    if (*argc > 2) {
      free((*argv)[1]);
      memmove(&(*argv)[1], &(*argv)[2], sizeof(char *) * (*argc - 2));
      (*argv)[(*argc) - 1] = NULL;
    }
    (*argc)--;
    data->argc = argc;
    data->argv = argv;
    DEBUG2("<%d> New process in the game: %p", index, proc);
  }
}

void smpi_process_destroy(void)
{
  int index = smpi_process_index();

  DEBUG1("<%d> Process left the game", index);
}

int smpi_process_argc(void) {
  smpi_process_data_t data = smpi_process_data();

  return data->argc ? *(data->argc) - 1 : 0;
}

int smpi_process_getarg(integer* index, char* dst, ftnlen len) {
  smpi_process_data_t data = smpi_process_data();
  char* arg;
  size_t i;

  if(!data->argc || !data->argv
     || *index < 1 || *index >= *(data->argc)) {
    return -1;
  }
  arg = (*data->argv)[*index];
  for(i = 0; i < len && arg[i] != '\0'; i++) {
    dst[i] = arg[i];
  }
  for(; i < len; i++) {
    dst[i] = ' ';
  }
  return 0;
}

int smpi_global_rank(void) {
   return smpi_process_index();
}

int smpi_global_size(void) {
   char* value = getenv("SMPI_GLOBAL_SIZE");

   if(!value) {
      abort();
   }
   return atoi(value);
}

smpi_process_data_t smpi_process_data(void)
{
  return SIMIX_process_get_data(SIMIX_process_self());
}

smpi_process_data_t smpi_process_remote_data(int index)
{
  return process_data[index];
}

int smpi_process_count(void)
{
  return process_count;
}

int smpi_process_index(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->index;
}

xbt_os_timer_t smpi_process_timer(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->timer;
}

void smpi_process_simulated_start(void)
{
  smpi_process_data_t data = smpi_process_data();

  data->simulated = SIMIX_get_clock();
}

double smpi_process_simulated_elapsed(void)
{
  smpi_process_data_t data = smpi_process_data();

  return SIMIX_get_clock() - data->simulated;
}

MPI_Comm smpi_process_comm_self(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->comm_self;
}

void print_request(const char *message, MPI_Request request)
{
  char *req =
      bprintf
      ("[buf = %p, size = %zu, src = %d, dst = %d, tag= %d, complete = %d, flags = %u]",
       request->buf, request->size, request->src, request->dst,
       request->tag, request->complete, request->flags);

  DEBUG5("%s  (request %p with rdv %p and match %p) %s",
         message, request, request->rdv, request->match, req);
  free(req);
}

void smpi_process_post_send(MPI_Comm comm, MPI_Request request)
{
  int index = smpi_group_index(smpi_comm_group(comm), request->dst);
  smpi_process_data_t data = smpi_process_remote_data(index);
  xbt_fifo_item_t item;
  MPI_Request req;

  print_request("Isend", request);
  xbt_fifo_foreach(data->pending_recv, item, req, MPI_Request) {
    if (req->comm == request->comm
        && (req->src == MPI_ANY_SOURCE || req->src == request->src)
        && (req->tag == MPI_ANY_TAG || req->tag == request->tag)) {
      print_request("Match found", req);
      xbt_fifo_remove_item(data->pending_recv, item);
      /* Materialize the *_ANY_* fields from corresponding irecv request */
      req->src = request->src;
      req->tag = request->tag;
      req->match = request;
      request->rdv = req->rdv;
      request->match = req;
      return;
    }
  }
  request->rdv = SIMIX_rdv_create(NULL);
  xbt_fifo_push(data->pending_sent, request);
}

void smpi_process_post_recv(MPI_Request request)
{
  smpi_process_data_t data = smpi_process_data();
  xbt_fifo_item_t item;
  MPI_Request req;

  print_request("Irecv", request);
  xbt_fifo_foreach(data->pending_sent, item, req, MPI_Request) {
    if (req->comm == request->comm
        && (request->src == MPI_ANY_SOURCE || req->src == request->src)
        && (request->tag == MPI_ANY_TAG || req->tag == request->tag)) {
      print_request("Match found", req);
      xbt_fifo_remove_item(data->pending_sent, item);
      /* Materialize the *_ANY_* fields from the irecv request */
      req->match = request;
      request->src = req->src;
      request->tag = req->tag;
      request->rdv = req->rdv;
      request->match = req;
      return;
    }
  }
  request->rdv = SIMIX_rdv_create(NULL);
  xbt_fifo_push(data->pending_recv, request);
}

void smpi_global_init(void)
{
  int i;
  MPI_Group group;

  SIMIX_network_set_copy_data_callback
      (&SIMIX_network_copy_buffer_callback);
  process_count = SIMIX_process_count();
  process_data = xbt_new(smpi_process_data_t, process_count);
  for (i = 0; i < process_count; i++) {
    process_data[i] = xbt_new(s_smpi_process_data_t, 1);
    process_data[i]->index = i;
    process_data[i]->argc = NULL;
    process_data[i]->argv = NULL;
    process_data[i]->pending_sent = xbt_fifo_new();
    process_data[i]->pending_recv = xbt_fifo_new();
    process_data[i]->timer = xbt_os_timer_new();
    group = smpi_group_new(1);
    process_data[i]->comm_self = smpi_comm_new(group);
    smpi_group_set_mapping(group, i, 0);
  }
  group = smpi_group_new(process_count);
  MPI_COMM_WORLD = smpi_comm_new(group);
  for (i = 0; i < process_count; i++) {
    smpi_group_set_mapping(group, i, i);
  }
}

void smpi_global_destroy(void)
{
  int count = smpi_process_count();
  int i;

  smpi_bench_destroy();
  smpi_comm_destroy(MPI_COMM_WORLD);
  MPI_COMM_WORLD = MPI_COMM_NULL;
  for (i = 0; i < count; i++) {
    smpi_comm_destroy(process_data[i]->comm_self);
    xbt_os_timer_free(process_data[i]->timer);
    xbt_fifo_free(process_data[i]->pending_recv);
    xbt_fifo_free(process_data[i]->pending_sent);
    xbt_free(process_data[i]);
  }
  xbt_free(process_data);
  process_data = NULL;
}

/* Fortran specific stuff */
/* With smpicc, the following weak symbols are used */
/* With smpiff, the following weak symbols are replaced by those in libf2c */
int __attribute__((weak)) xargc;
char** __attribute__((weak)) xargv;

int __attribute__((weak)) main(int argc, char** argv) {
   xargc = argc;
   xargv = argv;
   return MAIN__();
}

int MAIN__(void)
{
  srand(SMPI_RAND_SEED);

  double default_reference_speed = 20000.0;
  xbt_cfg_register(&_surf_cfg_set, "smpi/running_power",
                   "Power of the host running the simulation (in flop/s). Used to bench the operations.",
                   xbt_cfgelm_double, &default_reference_speed, 1, 1, NULL,
                   NULL);

  int default_display_timing = 0;
  xbt_cfg_register(&_surf_cfg_set, "smpi/display_timing",
                   "Boolean indicating whether we should display the timing after simulation.",
                   xbt_cfgelm_int, &default_display_timing, 1, 1, NULL,
                   NULL);

  int default_display_smpe = 0;
  xbt_cfg_register(&_surf_cfg_set, "smpi/log_events",
                   "Boolean indicating whether we should display simulated time spent in MPI calls.",
                   xbt_cfgelm_int, &default_display_smpe, 1, 1, NULL,
                   NULL);

  double default_threshold = 1e-6;
  xbt_cfg_register(&_surf_cfg_set, "smpi/cpu_threshold",
                   "Minimal computation time (in seconds) not discarded.",
                   xbt_cfgelm_double, &default_threshold, 1, 1, NULL,
                   NULL);

#ifdef HAVE_TRACING
  TRACE_global_init(&xargc, xargv);
#endif

  SIMIX_global_init(&xargc, xargv);

#ifdef HAVE_TRACING
  TRACE_smpi_start();
#endif

  // parse the platform file: get the host list
  SIMIX_create_environment(xargv[1]);

  SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
  SIMIX_launch_application(xargv[2]);

  smpi_global_init();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);
  SIMIX_init();

#ifdef HAVE_MC
  if (_surf_do_model_check)
    MC_modelcheck(1);
  else
#endif
    while (SIMIX_solve(NULL, NULL) != -1.0);

  if (xbt_cfg_get_int(_surf_cfg_set, "smpi/display_timing"))
    INFO1("simulation time %g", SIMIX_get_clock());

  smpi_global_destroy();

#ifdef HAVE_TRACING
  TRACE_smpi_release();
#endif

  SIMIX_clean();
  return 0;
}
