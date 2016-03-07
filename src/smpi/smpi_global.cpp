/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_mpi_dt_private.h"
#include "mc/mc.h"
#include "src/mc/mc_record.h"
#include "xbt/replay.h"
#include "surf/surf.h"
#include "src/simix/smx_private.h"
#include "simgrid/sg_config.h"
#include "src/mc/mc_replay.h"
#include "src/msg/msg_private.h"

#include <float.h>              /* DBL_MAX */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi, "Logging specific to SMPI (kernel)");

typedef struct s_smpi_process_data {
  double simulated;
  int *argc;
  char ***argv;
  smx_mailbox_t mailbox;
  smx_mailbox_t mailbox_small;
  xbt_mutex_t mailboxes_mutex;
  xbt_os_timer_t timer;
  MPI_Comm comm_self;
  MPI_Comm comm_intra;
  MPI_Comm* comm_world;
  void *data;                   /* user data */
  int index;
  char state;
  int sampling;                 /* inside an SMPI_SAMPLE_ block? */
  char* instance_id;
  int replaying;                /* is the process replaying a trace */
  xbt_bar_t finalization_barrier;
} s_smpi_process_data_t;

static smpi_process_data_t *process_data = NULL;
int process_count = 0;
int smpi_universe_size = 0;
int* index_to_process_data = NULL;
extern double smpi_total_benched_time;
xbt_os_timer_t global_timer;
MPI_Comm MPI_COMM_WORLD = MPI_COMM_UNINITIALIZED;

MPI_Errhandler *MPI_ERRORS_RETURN = NULL;
MPI_Errhandler *MPI_ERRORS_ARE_FATAL = NULL;
MPI_Errhandler *MPI_ERRHANDLER_NULL = NULL;

#define MAILBOX_NAME_MAXLEN (5 + sizeof(int) * 2 + 1)

static char *get_mailbox_name(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "SMPI-%0*x", (int) (sizeof(int) * 2), index);
  return str;
}

static char *get_mailbox_name_small(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "small%0*x", (int) (sizeof(int) * 2), index);
  return str;
}

void smpi_process_init(int *argc, char ***argv)
{
  int index=-1;
  smpi_process_data_t data;
  smx_process_t proc;

  if (argc && argv) {
    proc = SIMIX_process_self();
    //FIXME: dirty cleanup method to avoid using msg cleanup functions on these processes when using MSG+SMPI
    SIMIX_process_set_cleanup_function(proc, SIMIX_process_cleanup);
    char* instance_id = (*argv)[1];
    int rank = xbt_str_parse_int((*argv)[2], "Invalid rank: %s");
    index = smpi_process_index_of_smx_process(proc);

    if(!index_to_process_data){
      index_to_process_data=(int*)xbt_malloc(SIMIX_process_count()*sizeof(int));
    }

    if(smpi_privatize_global_variables){
      /* Now using segment index of the process  */
      index = proc->segment_index;
      /* Done at the process's creation */
      SMPI_switch_data_segment(index);
    }

    MPI_Comm* temp_comm_world;
    xbt_bar_t temp_bar;
    smpi_deployment_register_process(instance_id, rank, index, &temp_comm_world, &temp_bar);
    data              = smpi_process_remote_data(index);
    data->comm_world  = temp_comm_world;
    if(temp_bar != NULL) data->finalization_barrier = temp_bar;
    data->index       = index;
    data->instance_id = instance_id;
    data->replaying   = 0;
    //xbt_free(simcall_process_get_data(proc));

    simdata_process_t simdata = static_cast<simdata_process_t>(simcall_process_get_data(proc));
    simdata->data             = data;

    if (*argc > 3) {
      free((*argv)[1]);
      memmove(&(*argv)[0], &(*argv)[2], sizeof(char *) * (*argc - 2));
      (*argv)[(*argc) - 1] = NULL;
      (*argv)[(*argc) - 2] = NULL;
    }
    (*argc)-=2;
    data->argc = argc;
    data->argv = argv;
    // set the process attached to the mailbox
    simcall_rdv_set_receiver(data->mailbox_small, proc);
    XBT_DEBUG("<%d> New process in the game: %p", index, proc);
  }
  xbt_assert(smpi_process_data(),
      "smpi_process_data() returned NULL. You probably gave a NULL parameter to MPI_Init. Although it's required by "
      "MPI-2, this is currently not supported by SMPI.");
}

void smpi_process_destroy(void)
{
  int index = smpi_process_index();
  if(smpi_privatize_global_variables){
    smpi_switch_data_segment(index);
  }
  process_data[index_to_process_data[index]]->state = SMPI_FINALIZED;
  XBT_DEBUG("<%d> Process left the game", index);
}

/** @brief Prepares the current process for termination. */
void smpi_process_finalize(void)
{
    // This leads to an explosion of the search graph which cannot be reduced:
    if(MC_is_active() || MC_record_replay_is_active())
      return;

    int index = smpi_process_index();
    // wait for all pending asynchronous comms to finish
    xbt_barrier_wait(process_data[index_to_process_data[index]]->finalization_barrier);
}

/** @brief Check if a process is finalized */
int smpi_process_finalized()
{
  int index = smpi_process_index();
    if (index != MPI_UNDEFINED)
      return (process_data[index_to_process_data[index]]->state == SMPI_FINALIZED);
    else
      return 0;
}

/** @brief Check if a process is initialized */
int smpi_process_initialized(void)
{
  if (!index_to_process_data){
    return false;
  } else{
    int index = smpi_process_index();
    return ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state == SMPI_INITIALIZED));
  }
}

/** @brief Mark a process as initialized (=MPI_Init called) */
void smpi_process_mark_as_initialized(void)
{
  int index = smpi_process_index();
  if ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state != SMPI_FINALIZED))
    process_data[index_to_process_data[index]]->state = SMPI_INITIALIZED;
}

void smpi_process_set_replaying(int value){
  int index = smpi_process_index();
  if ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state != SMPI_FINALIZED))
    process_data[index_to_process_data[index]]->replaying = value;
}

int smpi_process_get_replaying(){
  int index = smpi_process_index();
  if (index != MPI_UNDEFINED)
    return process_data[index_to_process_data[index]]->replaying;
  else return _xbt_replay_is_active();
}

int smpi_global_size(void)
{
  char *value = getenv("SMPI_GLOBAL_SIZE");
  xbt_assert(value,"Please set env var SMPI_GLOBAL_SIZE to the expected number of processes.");

  return xbt_str_parse_int(value, "SMPI_GLOBAL_SIZE contains a non-numerical value: %s");
}

smpi_process_data_t smpi_process_data(void)
{
  simdata_process_t simdata = static_cast<simdata_process_t>(SIMIX_process_self_get_data());
  return static_cast<smpi_process_data_t>(simdata->data);
}

smpi_process_data_t smpi_process_remote_data(int index)
{
  return process_data[index_to_process_data[index]];
}

void smpi_process_set_user_data(void *data)
{
  smpi_process_data_t process_data = smpi_process_data();
  process_data->data = data;
}

void *smpi_process_get_user_data()
{
  smpi_process_data_t process_data = smpi_process_data();
  return process_data->data;
}

int smpi_process_count(void)
{
  return process_count;
}

int smpi_process_index(void)
{
  smpi_process_data_t data = smpi_process_data();
  //return -1 if not initialized
  return data ? data->index : MPI_UNDEFINED;
}

MPI_Comm smpi_process_comm_world(void)
{
  smpi_process_data_t data = smpi_process_data();
  //return MPI_COMM_NULL if not initialized
  return data ? *data->comm_world : MPI_COMM_NULL;
}

smx_mailbox_t smpi_process_mailbox(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailbox;
}

smx_mailbox_t smpi_process_mailbox_small(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailbox_small;
}

xbt_mutex_t smpi_process_mailboxes_mutex(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailboxes_mutex;
}

smx_mailbox_t smpi_process_remote_mailbox(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailbox;
}

smx_mailbox_t smpi_process_remote_mailbox_small(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailbox_small;
}

xbt_mutex_t smpi_process_remote_mailboxes_mutex(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailboxes_mutex;
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
  if(data->comm_self==MPI_COMM_NULL){
    MPI_Group group = smpi_group_new(1);
    data->comm_self = smpi_comm_new(group, NULL);
    smpi_group_set_mapping(group, smpi_process_index(), 0);
  }

  return data->comm_self;
}

MPI_Comm smpi_process_get_comm_intra(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->comm_intra;
}

void smpi_process_set_comm_intra(MPI_Comm comm)
{
  smpi_process_data_t data = smpi_process_data();
  data->comm_intra = comm;
}

void smpi_process_set_sampling(int s)
{
  smpi_process_data_t data = smpi_process_data();
  data->sampling = s;
}

int smpi_process_get_sampling(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->sampling;
}

void print_request(const char *message, MPI_Request request)
{
  XBT_VERB("%s  request %p  [buf = %p, size = %zu, src = %d, dst = %d, tag = %d, flags = %x]",
       message, request, request->buf, request->size, request->src, request->dst, request->tag, request->flags);
}

void smpi_comm_copy_buffer_callback(smx_synchro_t comm, void *buff, size_t buff_size)
{
  XBT_DEBUG("Copy the data over");
  void* tmpbuff=buff;

  if((smpi_privatize_global_variables) && ((char*)buff >= smpi_start_data_exe)
      && ((char*)buff < smpi_start_data_exe + smpi_size_data_exe )
    ){
       XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");
       smpi_switch_data_segment(((smpi_process_data_t)(((simdata_process_t)SIMIX_process_get_data(comm->comm.src_proc))->data))->index);
       tmpbuff = (void*)xbt_malloc(buff_size);
       memcpy(tmpbuff, buff, buff_size);
  }

  if((smpi_privatize_global_variables) && ((char*)comm->comm.dst_buff >= smpi_start_data_exe)
      && ((char*)comm->comm.dst_buff < smpi_start_data_exe + smpi_size_data_exe )){
       XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");
       smpi_switch_data_segment(((smpi_process_data_t)(((simdata_process_t)SIMIX_process_get_data(comm->comm.dst_proc))->data))->index);
  }

  memcpy(comm->comm.dst_buff, tmpbuff, buff_size);
  if (comm->comm.detached) {
    // if this is a detached send, the source buffer was duplicated by SMPI
    // sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    //It seems that the request is used after the call there this should be free somewhere else but where???
    //xbt_free(comm->comm.src_data);// inside SMPI the request is kept inside the user data and should be free
    comm->comm.src_buff = NULL;
  }

  if(tmpbuff!=buff)xbt_free(tmpbuff);
}

void smpi_comm_null_copy_buffer_callback(smx_synchro_t comm, void *buff, size_t buff_size)
{
  return;
}

static void smpi_check_options(){
  //check correctness of MPI parameters

   xbt_assert(sg_cfg_get_int("smpi/async_small_thresh") <= sg_cfg_get_int("smpi/send_is_detached_thresh"));

   if (sg_cfg_is_default_value("smpi/running_power")) {
     XBT_INFO("You did not set the power of the host running the simulation.  "
              "The timings will certainly not be accurate.  "
              "Use the option \"--cfg=smpi/running_power:<flops>\" to set its value."
              "Check http://simgrid.org/simgrid/latest/doc/options.html#options_smpi_bench for more information.");
   }
}

int smpi_enabled(void) {
  return process_data != NULL;
}

void smpi_global_init(void)
{
  int i;
  MPI_Group group;
  char name[MAILBOX_NAME_MAXLEN];
  int smpirun=0;

  if (!MC_is_active()) {
    global_timer = xbt_os_timer_new();
    xbt_os_walltimer_start(global_timer);
  }
  if (process_count == 0){
    process_count = SIMIX_process_count();
    smpirun=1;
  }
  smpi_universe_size = process_count;
  process_data = xbt_new0(smpi_process_data_t, process_count);
  for (i = 0; i < process_count; i++) {
    process_data[i]                       = xbt_new(s_smpi_process_data_t, 1);
    //process_data[i]->index              = i;
    process_data[i]->argc                 = NULL;
    process_data[i]->argv                 = NULL;
    process_data[i]->mailbox              = simcall_rdv_create(get_mailbox_name(name, i));
    process_data[i]->mailbox_small        = simcall_rdv_create(get_mailbox_name_small(name, i));
    process_data[i]->mailboxes_mutex      = xbt_mutex_init();
    process_data[i]->timer                = xbt_os_timer_new();
    if (MC_is_active())
      MC_ignore_heap(process_data[i]->timer, xbt_os_timer_size());
    process_data[i]->comm_self            = MPI_COMM_NULL;
    process_data[i]->comm_intra           = MPI_COMM_NULL;
    process_data[i]->comm_world           = NULL;
    process_data[i]->state                = SMPI_UNINITIALIZED;
    process_data[i]->sampling             = 0;
    process_data[i]->finalization_barrier = NULL;
  }
  //if the process was launched through smpirun script we generate a global mpi_comm_world
  //if not, we let MPI_COMM_NULL, and the comm world will be private to each mpi instance
  if(smpirun){
    group = smpi_group_new(process_count);
    MPI_COMM_WORLD = smpi_comm_new(group, NULL);
    MPI_Attr_put(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, (void *)(MPI_Aint)process_count);
    xbt_bar_t bar=xbt_barrier_init(process_count);

    for (i = 0; i < process_count; i++) {
      smpi_group_set_mapping(group, i, i);
      process_data[i]->finalization_barrier = bar;
    }
  }
}

void smpi_global_destroy(void)
{
  int count = smpi_process_count();
  int i;

  smpi_bench_destroy();
  if (MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED){
      while (smpi_group_unuse(smpi_comm_group(MPI_COMM_WORLD)) > 0);
      xbt_free(MPI_COMM_WORLD);
      xbt_barrier_destroy(process_data[0]->finalization_barrier);
  }else{
      smpi_deployment_cleanup_instances();
  }
  MPI_COMM_WORLD = MPI_COMM_NULL;
  for (i = 0; i < count; i++) {
    if(process_data[i]->comm_self!=MPI_COMM_NULL){
      smpi_group_unuse(smpi_comm_group(process_data[i]->comm_self));
      smpi_comm_destroy(process_data[i]->comm_self);
    }
    if(process_data[i]->comm_intra!=MPI_COMM_NULL){
      smpi_group_unuse(smpi_comm_group(process_data[i]->comm_intra));
      smpi_comm_destroy(process_data[i]->comm_intra);
    }
    xbt_os_timer_free(process_data[i]->timer);
    simcall_rdv_destroy(process_data[i]->mailbox);
    simcall_rdv_destroy(process_data[i]->mailbox_small);
    xbt_mutex_destroy(process_data[i]->mailboxes_mutex);
    xbt_free(process_data[i]);
  }
  xbt_free(process_data);
  process_data = NULL;

  xbt_free(index_to_process_data);
  if(smpi_privatize_global_variables)
    smpi_destroy_global_memory_segments();
  smpi_free_static();
}

#ifndef WIN32
void __attribute__ ((weak)) user_main_()
{
  xbt_die("Should not be in this smpi_simulated_main");
  return;
}

int __attribute__ ((weak)) smpi_simulated_main_(int argc, char **argv)
{
  smpi_process_init(&argc, &argv);
  user_main_();
  return 0;
}

int __attribute__ ((weak)) main(int argc, char **argv)
{
  return smpi_main(smpi_simulated_main_, argc, argv);
}

#endif

extern "C" {
static void smpi_init_logs(){

  /* Connect log categories.  See xbt/log.c */

  XBT_LOG_CONNECT(smpi);  /* Keep this line as soon as possible in this function: xbt_log_appender_file.c depends on it
                             DO NOT connect this in XBT or so, or it will be useless to xbt_log_appender_file.c */
  XBT_LOG_CONNECT(instr_smpi);
  XBT_LOG_CONNECT(smpi_base);
  XBT_LOG_CONNECT(smpi_bench);
  XBT_LOG_CONNECT(smpi_coll);
  XBT_LOG_CONNECT(smpi_colls);
  XBT_LOG_CONNECT(smpi_comm);
  XBT_LOG_CONNECT(smpi_dvfs);
  XBT_LOG_CONNECT(smpi_group);
  XBT_LOG_CONNECT(smpi_kernel);
  XBT_LOG_CONNECT(smpi_mpi);
  XBT_LOG_CONNECT(smpi_mpi_dt);
  XBT_LOG_CONNECT(smpi_pmpi);
  XBT_LOG_CONNECT(smpi_replay);
  XBT_LOG_CONNECT(smpi_rma);
}
}

static void smpi_init_options(){
  int gather_id = find_coll_description(mpi_coll_gather_description, sg_cfg_get_string("smpi/gather"),"gather");
    mpi_coll_gather_fun = (int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm))
        mpi_coll_gather_description[gather_id].coll;

    int allgather_id = find_coll_description(mpi_coll_allgather_description,
                                             sg_cfg_get_string("smpi/allgather"),"allgather");
    mpi_coll_allgather_fun = (int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm))
        mpi_coll_allgather_description[allgather_id].coll;

    int allgatherv_id = find_coll_description(mpi_coll_allgatherv_description,
                                              sg_cfg_get_string("smpi/allgatherv"),"allgatherv");
    mpi_coll_allgatherv_fun = (int (*)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm))
        mpi_coll_allgatherv_description[allgatherv_id].coll;

    int allreduce_id = find_coll_description(mpi_coll_allreduce_description,
                                             sg_cfg_get_string("smpi/allreduce"),"allreduce");
    mpi_coll_allreduce_fun = (int (*)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm))
        mpi_coll_allreduce_description[allreduce_id].coll;

    int alltoall_id = find_coll_description(mpi_coll_alltoall_description,
                                            sg_cfg_get_string("smpi/alltoall"),"alltoall");
    mpi_coll_alltoall_fun = (int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm))
        mpi_coll_alltoall_description[alltoall_id].coll;

    int alltoallv_id = find_coll_description(mpi_coll_alltoallv_description,
                                             sg_cfg_get_string("smpi/alltoallv"),"alltoallv");
    mpi_coll_alltoallv_fun = (int (*)(void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm))
        mpi_coll_alltoallv_description[alltoallv_id].coll;

    int bcast_id = find_coll_description(mpi_coll_bcast_description, sg_cfg_get_string("smpi/bcast"),"bcast");
    mpi_coll_bcast_fun = (int (*)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com))
        mpi_coll_bcast_description[bcast_id].coll;

    int reduce_id = find_coll_description(mpi_coll_reduce_description, sg_cfg_get_string("smpi/reduce"),"reduce");
    mpi_coll_reduce_fun = (int (*)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                                    MPI_Comm comm)) mpi_coll_reduce_description[reduce_id].coll;

    int reduce_scatter_id =
        find_coll_description(mpi_coll_reduce_scatter_description,
                              sg_cfg_get_string("smpi/reduce_scatter"),"reduce_scatter");
    mpi_coll_reduce_scatter_fun = (int (*)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)) mpi_coll_reduce_scatter_description[reduce_scatter_id].coll;

    int scatter_id = find_coll_description(mpi_coll_scatter_description, sg_cfg_get_string("smpi/scatter"),"scatter");
    mpi_coll_scatter_fun = (int (*)(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm))
        mpi_coll_scatter_description[scatter_id].coll;

    int barrier_id = find_coll_description(mpi_coll_barrier_description, sg_cfg_get_string("smpi/barrier"),"barrier");
    mpi_coll_barrier_fun = (int (*)(MPI_Comm comm)) mpi_coll_barrier_description[barrier_id].coll;

    smpi_cpu_threshold = sg_cfg_get_double("smpi/cpu_threshold");
    smpi_running_power = sg_cfg_get_double("smpi/running_power");
    smpi_privatize_global_variables = sg_cfg_get_boolean("smpi/privatize_global_variables");
    if (smpi_cpu_threshold < 0)
      smpi_cpu_threshold = DBL_MAX;
}

int smpi_main(int (*realmain) (int argc, char *argv[]), int argc, char *argv[])
{
  srand(SMPI_RAND_SEED);

  if (getenv("SMPI_PRETEND_CC") != NULL) {
    /* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the
     * configuration tools */
    return 0;
  }
  smpi_init_logs();

  TRACE_global_init(&argc, argv);
  TRACE_add_start_function(TRACE_smpi_alloc);
  TRACE_add_end_function(TRACE_smpi_release);

  SIMIX_global_init(&argc, argv);
  MSG_init(&argc,argv);

  SMPI_switch_data_segment = smpi_switch_data_segment;

  smpi_init_options();

  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);
  SIMIX_comm_set_copy_data_callback(&smpi_comm_copy_buffer_callback);
  SIMIX_function_register_default(realmain);
  SIMIX_launch_application(argv[2]);

  smpi_global_init();

  smpi_check_options();

  if(smpi_privatize_global_variables)
    smpi_initialize_global_memory_segments();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
  
    SIMIX_run();

    xbt_os_walltimer_stop(global_timer);
    if (sg_cfg_get_boolean("smpi/display_timing")){
      double global_time = xbt_os_timer_elapsed(global_timer);
      XBT_INFO("Simulated time: %g seconds. \n\n"
          "The simulation took %g seconds (after parsing and platform setup)\n"
          "%g seconds were actual computation of the application",
          SIMIX_get_clock(), global_time , smpi_total_benched_time);
          
      if (smpi_total_benched_time/global_time>=0.75)
      XBT_INFO("More than 75%% of the time was spent inside the application code.\n"
      "You may want to use sampling functions or trace replay to reduce this.");
    }
  }
  smpi_global_destroy();

  TRACE_end();

  return 0;
}

// This function can be called from extern file, to initialize logs, options, and processes of smpi
// without the need of smpirun
void SMPI_init(){
  smpi_init_logs();
  smpi_init_options();
  smpi_global_init();
  smpi_check_options();
  if (TRACE_is_enabled() && TRACE_is_configured())
    TRACE_smpi_alloc();
  if(smpi_privatize_global_variables)
    smpi_initialize_global_memory_segments();
}

void SMPI_finalize(){
  smpi_global_destroy();
}
