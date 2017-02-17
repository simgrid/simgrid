/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>

#include "mc/mc.h"
#include "private.h"
#include "private.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/sg_config.h"
#include "smpi_mpi_dt_private.h"
#include "src/kernel/activity/SynchroComm.hpp"
#include "src/mc/mc_record.h"
#include "src/mc/mc_replay.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_private.h"
#include "surf/surf.h"
#include "xbt/replay.h"

#include <float.h> /* DBL_MAX */
#include <fstream>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi, "Logging specific to SMPI (kernel)");
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp> /* trim_right / trim_left */

#if HAVE_PAPI
#include "papi.h"
const char* papi_default_config_name = "default";

struct papi_process_data {
  papi_counter_t counter_data;
  int event_set;
};

#endif
std::unordered_map<std::string, double> location2speedup;

typedef struct s_smpi_process_data {
  double simulated;
  int *argc;
  char ***argv;
  simgrid::s4u::MailboxPtr mailbox;
  simgrid::s4u::MailboxPtr mailbox_small;
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
  bool replaying;                /* is the process replaying a trace */
  msg_bar_t finalization_barrier;
  int return_value;
  smpi_trace_call_location_t trace_call_loc;
#if HAVE_PAPI
  /** Contains hardware data as read by PAPI **/
  int papi_event_set;
  papi_counter_t papi_counter_data;
#endif
} s_smpi_process_data_t;

static smpi_process_data_t *process_data = nullptr;
int process_count = 0;
int smpi_universe_size = 0;
int* index_to_process_data = nullptr;
extern double smpi_total_benched_time;
extern xbt_dict_t smpi_type_keyvals;
extern xbt_dict_t smpi_comm_keyvals;
xbt_os_timer_t global_timer;
MPI_Comm MPI_COMM_WORLD = MPI_COMM_UNINITIALIZED;
MPI_Errhandler *MPI_ERRORS_RETURN = nullptr;
MPI_Errhandler *MPI_ERRORS_ARE_FATAL = nullptr;
MPI_Errhandler *MPI_ERRHANDLER_NULL = nullptr;

#define MAILBOX_NAME_MAXLEN (5 + sizeof(int) * 2 + 1)

static char *get_mailbox_name(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "SMPI-%0*x", static_cast<int> (sizeof(int) * 2), index);
  return str;
}

static char *get_mailbox_name_small(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "small%0*x", static_cast<int> (sizeof(int) * 2), index);
  return str;
}

void smpi_process_init(int *argc, char ***argv)
{

  if (process_data == nullptr){
    printf("SimGrid was not initialized properly before entering MPI_Init. Aborting, please check compilation process and use smpirun\n");
    exit(1);
  }
  if (argc != nullptr && argv != nullptr) {
    smx_actor_t proc = SIMIX_process_self();
    proc->context->set_cleanup(&MSG_process_cleanup_from_SIMIX);
    char* instance_id = (*argv)[1];
    int rank = xbt_str_parse_int((*argv)[2], "Invalid rank: %s");
    int index = smpi_process_index_of_smx_process(proc);

    if(index_to_process_data == nullptr){
      index_to_process_data=static_cast<int*>(xbt_malloc(SIMIX_process_count()*sizeof(int)));
    }

    if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
      /* Now using segment index of the process  */
      index = proc->segment_index;
      /* Done at the process's creation */
      SMPI_switch_data_segment(index);
    }

    MPI_Comm* temp_comm_world;
    msg_bar_t temp_bar;
    smpi_deployment_register_process(instance_id, rank, index, &temp_comm_world, &temp_bar);
    smpi_process_data_t data = smpi_process_remote_data(index);
    data->comm_world         = temp_comm_world;
    if(temp_bar != nullptr) 
      data->finalization_barrier = temp_bar;
    data->index       = index;
    data->instance_id = instance_id;
    data->replaying   = false;

    static_cast<simgrid::MsgActorExt*>(proc->data)->data = data;

    if (*argc > 3) {
      memmove(&(*argv)[0], &(*argv)[2], sizeof(char *) * (*argc - 2));
      (*argv)[(*argc) - 1] = nullptr;
      (*argv)[(*argc) - 2] = nullptr;
    }
    (*argc)-=2;
    data->argc = argc;
    data->argv = argv;
    // set the process attached to the mailbox
    data->mailbox_small->setReceiver(simgrid::s4u::Actor::self());
    XBT_DEBUG("<%d> New process in the game: %p", index, proc);
  }
  xbt_assert(smpi_process_data(),
      "smpi_process_data() returned nullptr. You probably gave a nullptr parameter to MPI_Init. "
      "Although it's required by MPI-2, this is currently not supported by SMPI.");
}

void smpi_process_destroy()
{
  int index = smpi_process_index();
  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
    smpi_switch_data_segment(index);
  }
  process_data[index_to_process_data[index]]->state = SMPI_FINALIZED;
  XBT_DEBUG("<%d> Process left the game", index);
}

/** @brief Prepares the current process for termination. */
void smpi_process_finalize()
{
    // This leads to an explosion of the search graph which cannot be reduced:
    if(MC_is_active() || MC_record_replay_is_active())
      return;

    int index = smpi_process_index();
    // wait for all pending asynchronous comms to finish
    MSG_barrier_wait(process_data[index_to_process_data[index]]->finalization_barrier);
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
int smpi_process_initialized()
{
  if (index_to_process_data == nullptr){
    return false;
  } else{
    int index = smpi_process_index();
    return ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state == SMPI_INITIALIZED));
  }
}

/** @brief Mark a process as initialized (=MPI_Init called) */
void smpi_process_mark_as_initialized()
{
  int index = smpi_process_index();
  if ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state != SMPI_FINALIZED))
    process_data[index_to_process_data[index]]->state = SMPI_INITIALIZED;
}

void smpi_process_set_replaying(bool value){
  int index = smpi_process_index();
  if ((index != MPI_UNDEFINED) && (process_data[index_to_process_data[index]]->state != SMPI_FINALIZED))
    process_data[index_to_process_data[index]]->replaying = value;
}

bool smpi_process_get_replaying(){
  int index = smpi_process_index();
  if (index != MPI_UNDEFINED)
    return process_data[index_to_process_data[index]]->replaying;
  else return (_xbt_replay_is_active() != 0);
}

int smpi_global_size()
{
  char *value = getenv("SMPI_GLOBAL_SIZE");
  xbt_assert(value,"Please set env var SMPI_GLOBAL_SIZE to the expected number of processes.");

  return xbt_str_parse_int(value, "SMPI_GLOBAL_SIZE contains a non-numerical value: %s");
}

smpi_process_data_t smpi_process_data()
{
  simgrid::MsgActorExt* msgExt = static_cast<simgrid::MsgActorExt*>(SIMIX_process_self()->data);
  return static_cast<smpi_process_data_t>(msgExt->data);
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

int smpi_process_count()
{
  return process_count;
}

/**
 * \brief Returns a structure that stores the location (filename + linenumber)
 *        of the last calls to MPI_* functions.
 *
 * \see smpi_trace_set_call_location
 */
smpi_trace_call_location_t* smpi_process_get_call_location()
{
  smpi_process_data_t process_data = smpi_process_data();
  return &process_data->trace_call_loc;
}

int smpi_process_index()
{
  smpi_process_data_t data = smpi_process_data();
  //return -1 if not initialized
  return data != nullptr ? data->index : MPI_UNDEFINED;
}

MPI_Comm smpi_process_comm_world()
{
  smpi_process_data_t data = smpi_process_data();
  //return MPI_COMM_NULL if not initialized
  return data != nullptr ? *data->comm_world : MPI_COMM_NULL;
}

smx_mailbox_t smpi_process_mailbox()
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailbox->getImpl();
}

smx_mailbox_t smpi_process_mailbox_small()
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailbox_small->getImpl();
}

xbt_mutex_t smpi_process_mailboxes_mutex()
{
  smpi_process_data_t data = smpi_process_data();
  return data->mailboxes_mutex;
}

smx_mailbox_t smpi_process_remote_mailbox(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailbox->getImpl();
}

smx_mailbox_t smpi_process_remote_mailbox_small(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailbox_small->getImpl();
}

xbt_mutex_t smpi_process_remote_mailboxes_mutex(int index)
{
  smpi_process_data_t data = smpi_process_remote_data(index);
  return data->mailboxes_mutex;
}

#if HAVE_PAPI
int smpi_process_papi_event_set(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->papi_event_set;
}

papi_counter_t& smpi_process_papi_counters(void)
{
  smpi_process_data_t data = smpi_process_data();
  return data->papi_counter_data;
}
#endif

xbt_os_timer_t smpi_process_timer()
{
  smpi_process_data_t data = smpi_process_data();
  return data->timer;
}

void smpi_process_simulated_start()
{
  smpi_process_data_t data = smpi_process_data();
  data->simulated = SIMIX_get_clock();
}

double smpi_process_simulated_elapsed()
{
  smpi_process_data_t data = smpi_process_data();
  return SIMIX_get_clock() - data->simulated;
}

MPI_Comm smpi_process_comm_self()
{
  smpi_process_data_t data = smpi_process_data();
  if(data->comm_self==MPI_COMM_NULL){
    MPI_Group group = smpi_group_new(1);
    data->comm_self = smpi_comm_new(group, nullptr);
    smpi_group_set_mapping(group, smpi_process_index(), 0);
  }

  return data->comm_self;
}

MPI_Comm smpi_process_get_comm_intra()
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

int smpi_process_get_sampling()
{
  smpi_process_data_t data = smpi_process_data();
  return data->sampling;
}

void print_request(const char *message, MPI_Request request)
{
  XBT_VERB("%s  request %p  [buf = %p, size = %zu, src = %d, dst = %d, tag = %d, flags = %x]",
       message, request, request->buf, request->size, request->src, request->dst, request->tag, request->flags);
}

void smpi_comm_copy_buffer_callback(smx_activity_t synchro, void *buff, size_t buff_size)
{
  XBT_DEBUG("Copy the data over");
  void* tmpbuff=buff;
  simgrid::kernel::activity::Comm *comm = dynamic_cast<simgrid::kernel::activity::Comm*>(synchro);

  if((smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) && (static_cast<char*>(buff) >= smpi_start_data_exe)
      && (static_cast<char*>(buff) < smpi_start_data_exe + smpi_size_data_exe )
    ){
       XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");

       smpi_switch_data_segment(
           (static_cast<smpi_process_data_t>((static_cast<simgrid::MsgActorExt*>(comm->src_proc->data)->data))->index));
       tmpbuff = static_cast<void*>(xbt_malloc(buff_size));
       memcpy(tmpbuff, buff, buff_size);
  }

  if((smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) && ((char*)comm->dst_buff >= smpi_start_data_exe)
      && ((char*)comm->dst_buff < smpi_start_data_exe + smpi_size_data_exe )){
       XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");
       smpi_switch_data_segment(
           (static_cast<smpi_process_data_t>((static_cast<simgrid::MsgActorExt*>(comm->dst_proc->data)->data))->index));
  }

  memcpy(comm->dst_buff, tmpbuff, buff_size);
  if (comm->detached) {
    // if this is a detached send, the source buffer was duplicated by SMPI
    // sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    //It seems that the request is used after the call there this should be free somewhere else but where???
    //xbt_free(comm->comm.src_data);// inside SMPI the request is kept inside the user data and should be free
    comm->src_buff = nullptr;
  }

  if(tmpbuff!=buff)xbt_free(tmpbuff);
}

void smpi_comm_null_copy_buffer_callback(smx_activity_t comm, void *buff, size_t buff_size)
{
  /* nothing done in this version */
}

static void smpi_check_options(){
  //check correctness of MPI parameters

   xbt_assert(xbt_cfg_get_int("smpi/async-small-thresh") <= xbt_cfg_get_int("smpi/send-is-detached-thresh"));

   if (xbt_cfg_is_default_value("smpi/host-speed")) {
     XBT_INFO("You did not set the power of the host running the simulation.  "
              "The timings will certainly not be accurate.  "
              "Use the option \"--cfg=smpi/host-speed:<flops>\" to set its value."
              "Check http://simgrid.org/simgrid/latest/doc/options.html#options_smpi_bench for more information.");
   }

   xbt_assert(xbt_cfg_get_double("smpi/cpu-threshold") >=0,
       "The 'smpi/cpu-threshold' option cannot have negative values [anymore]. If you want to discard "
       "the simulation of any computation, please use 'smpi/simulate-computation:no' instead.");
}

int smpi_enabled() {
  return process_data != nullptr;
}

void smpi_global_init()
{
  int i;
  MPI_Group group;
  char name[MAILBOX_NAME_MAXLEN];
  int smpirun=0;

  if (!MC_is_active()) {
    global_timer = xbt_os_timer_new();
    xbt_os_walltimer_start(global_timer);
  }

  if (xbt_cfg_get_string("smpi/comp-adjustment-file")[0] != '\0') { 
    std::string filename {xbt_cfg_get_string("smpi/comp-adjustment-file")};
    std::ifstream fstream(filename);
    if (!fstream.is_open()) {
      xbt_die("Could not open file %s. Does it exist?", filename.c_str());
    }

    std::string line;
    typedef boost::tokenizer< boost::escaped_list_separator<char>> Tokenizer;
    std::getline(fstream, line); // Skip the header line
    while (std::getline(fstream, line)) {
      Tokenizer tok(line);
      Tokenizer::iterator it  = tok.begin();
      Tokenizer::iterator end = std::next(tok.begin());

      std::string location = *it;
      boost::trim(location);
      location2speedup.insert(std::pair<std::string, double>(location, std::stod(*end)));
    }
  }

#if HAVE_PAPI
  // This map holds for each computation unit (such as "default" or "process1" etc.)
  // the configuration as given by the user (counter data as a pair of (counter_name, counter_counter))
  // and the (computed) event_set.
  std::map</* computation unit name */ std::string, papi_process_data> units2papi_setup;

  if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0') {
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
      XBT_ERROR("Could not initialize PAPI library; is it correctly installed and linked?"
                " Expected version is %i",
                PAPI_VER_CURRENT);

    typedef boost::tokenizer<boost::char_separator<char>> Tokenizer;
    boost::char_separator<char> separator_units(";");
    std::string str = std::string(xbt_cfg_get_string("smpi/papi-events"));
    Tokenizer tokens(str, separator_units);

    // Iterate over all the computational units. This could be
    // processes, hosts, threads, ranks... You name it. I'm not exactly
    // sure what we will support eventually, so I'll leave it at the
    // general term "units".
    for (auto& unit_it : tokens) {
      boost::char_separator<char> separator_events(":");
      Tokenizer event_tokens(unit_it, separator_events);

      int event_set = PAPI_NULL;
      if (PAPI_create_eventset(&event_set) != PAPI_OK) {
        // TODO: Should this let the whole simulation die?
        XBT_CRITICAL("Could not create PAPI event set during init.");
      }

      // NOTE: We cannot use a map here, as we must obey the order of the counters
      // This is important for PAPI: We need to map the values of counters back
      // to the event_names (so, when PAPI_read() has finished)!
      papi_counter_t counters2values;

      // Iterate over all counters that were specified for this specific
      // unit.
      // Note that we need to remove the name of the unit
      // (that could also be the "default" value), which always comes first.
      // Hence, we start at ++(events.begin())!
      for (Tokenizer::iterator events_it = ++(event_tokens.begin()); events_it != event_tokens.end(); events_it++) {

        int event_code   = PAPI_NULL;
        char* event_name = const_cast<char*>((*events_it).c_str());
        if (PAPI_event_name_to_code(event_name, &event_code) == PAPI_OK) {
          if (PAPI_add_event(event_set, event_code) != PAPI_OK) {
            XBT_ERROR("Could not add PAPI event '%s'. Skipping.", event_name);
            continue;
          } else {
            XBT_DEBUG("Successfully added PAPI event '%s' to the event set.", event_name);
          }
        } else {
          XBT_CRITICAL("Could not find PAPI event '%s'. Skipping.", event_name);
          continue;
        }

        counters2values.push_back(
            // We cannot just pass *events_it, as this is of type const basic_string
            std::make_pair<std::string, long long>(std::string(*events_it), 0));
      }

      std::string unit_name    = *(event_tokens.begin());
      papi_process_data config = {.counter_data = std::move(counters2values), .event_set = event_set};

      units2papi_setup.insert(std::make_pair(unit_name, std::move(config)));
    }
  }
#endif
  if (process_count == 0){
    process_count = SIMIX_process_count();
    smpirun=1;
  }
  smpi_universe_size = process_count;
  process_data       = new smpi_process_data_t[process_count];
  for (i = 0; i < process_count; i++) {
    process_data[i]                       = new s_smpi_process_data_t;
    process_data[i]->argc                 = nullptr;
    process_data[i]->argv                 = nullptr;
    process_data[i]->mailbox              = simgrid::s4u::Mailbox::byName(get_mailbox_name(name, i));
    process_data[i]->mailbox_small        = simgrid::s4u::Mailbox::byName(get_mailbox_name_small(name, i));
    process_data[i]->mailboxes_mutex      = xbt_mutex_init();
    process_data[i]->timer                = xbt_os_timer_new();
    if (MC_is_active())
      MC_ignore_heap(process_data[i]->timer, xbt_os_timer_size());
    process_data[i]->comm_self            = MPI_COMM_NULL;
    process_data[i]->comm_intra           = MPI_COMM_NULL;
    process_data[i]->comm_world           = nullptr;
    process_data[i]->state                = SMPI_UNINITIALIZED;
    process_data[i]->sampling             = 0;
    process_data[i]->finalization_barrier = nullptr;
    process_data[i]->return_value         = 0;

#if HAVE_PAPI
    if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0') {
      // TODO: Implement host/process/thread based counters. This implementation
      // just always takes the values passed via "default", like this:
      // "default:COUNTER1:COUNTER2:COUNTER3;".
      auto it = units2papi_setup.find(papi_default_config_name);
      if (it != units2papi_setup.end()) {
        process_data[i]->papi_event_set    = it->second.event_set;
        process_data[i]->papi_counter_data = it->second.counter_data;
        XBT_DEBUG("Setting PAPI set for process %i", i);
      } else {
        process_data[i]->papi_event_set = PAPI_NULL;
        XBT_DEBUG("No PAPI set for process %i", i);
      }
    }
#endif
  }
  //if the process was launched through smpirun script we generate a global mpi_comm_world
  //if not, we let MPI_COMM_NULL, and the comm world will be private to each mpi instance
  if(smpirun){
    group = smpi_group_new(process_count);
    MPI_COMM_WORLD = smpi_comm_new(group, nullptr);
    MPI_Attr_put(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, reinterpret_cast<void *>(process_count));
    msg_bar_t bar = MSG_barrier_init(process_count);

    for (i = 0; i < process_count; i++) {
      smpi_group_set_mapping(group, i, i);
      process_data[i]->finalization_barrier = bar;
    }
  }
}

void smpi_global_destroy()
{
  int count = smpi_process_count();

  smpi_bench_destroy();
  if (MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED){
      while (smpi_group_unuse(smpi_comm_group(MPI_COMM_WORLD)) > 0);
      MSG_barrier_destroy(process_data[0]->finalization_barrier);
  }else{
      smpi_deployment_cleanup_instances();
  }
  for (int i = 0; i < count; i++) {
    if(process_data[i]->comm_self!=MPI_COMM_NULL){
      smpi_comm_destroy(process_data[i]->comm_self);
    }
    if(process_data[i]->comm_intra!=MPI_COMM_NULL){
      smpi_comm_destroy(process_data[i]->comm_intra);
    }
    xbt_os_timer_free(process_data[i]->timer);
    xbt_mutex_destroy(process_data[i]->mailboxes_mutex);
    delete process_data[i];
  }
  delete[] process_data;
  process_data = nullptr;

  if (MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED){
    smpi_comm_cleanup_smp(MPI_COMM_WORLD);
    smpi_comm_cleanup_attributes(MPI_COMM_WORLD);
    if(smpi_coll_cleanup_callback!=nullptr)
      smpi_coll_cleanup_callback();
    xbt_free(MPI_COMM_WORLD);
  }

  MPI_COMM_WORLD = MPI_COMM_NULL;

  if (!MC_is_active()) {
    xbt_os_timer_free(global_timer);
  }

  xbt_free(index_to_process_data);
  if(smpi_type_keyvals!=nullptr) 
    xbt_dict_free(&smpi_type_keyvals);
  if(smpi_comm_keyvals!=nullptr) 
    xbt_dict_free(&smpi_comm_keyvals);
  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP)
    smpi_destroy_global_memory_segments();
  smpi_free_static();
}

#ifndef WIN32

void __attribute__ ((weak)) user_main_()
{
  xbt_die("Should not be in this smpi_simulated_main");
}

int __attribute__ ((weak)) smpi_simulated_main_(int argc, char **argv)
{
  smpi_process_init(&argc, &argv);
  user_main_();
  return 0;
}

inline static int smpi_main_wrapper(int argc, char **argv){
  int ret = smpi_simulated_main_(argc,argv);
  if(ret !=0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", ret);
    smpi_process_data()->return_value=ret;
  }
  return 0;
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
  int gather_id = find_coll_description(mpi_coll_gather_description, xbt_cfg_get_string("smpi/gather"),"gather");
    mpi_coll_gather_fun = reinterpret_cast<int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm)>
        (mpi_coll_gather_description[gather_id].coll);

    int allgather_id = find_coll_description(mpi_coll_allgather_description,
                                             xbt_cfg_get_string("smpi/allgather"),"allgather");
    mpi_coll_allgather_fun = reinterpret_cast<int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm)>
        (mpi_coll_allgather_description[allgather_id].coll);

    int allgatherv_id = find_coll_description(mpi_coll_allgatherv_description,
                                              xbt_cfg_get_string("smpi/allgatherv"),"allgatherv");
    mpi_coll_allgatherv_fun = reinterpret_cast<int (*)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm)>
        (mpi_coll_allgatherv_description[allgatherv_id].coll);

    int allreduce_id = find_coll_description(mpi_coll_allreduce_description,
                                             xbt_cfg_get_string("smpi/allreduce"),"allreduce");
    mpi_coll_allreduce_fun = reinterpret_cast<int (*)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)>
        (mpi_coll_allreduce_description[allreduce_id].coll);

    int alltoall_id = find_coll_description(mpi_coll_alltoall_description,
                                            xbt_cfg_get_string("smpi/alltoall"),"alltoall");
    mpi_coll_alltoall_fun = reinterpret_cast<int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm)>
        (mpi_coll_alltoall_description[alltoall_id].coll);

    int alltoallv_id = find_coll_description(mpi_coll_alltoallv_description,
                                             xbt_cfg_get_string("smpi/alltoallv"),"alltoallv");
    mpi_coll_alltoallv_fun = reinterpret_cast<int (*)(void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm)>
        (mpi_coll_alltoallv_description[alltoallv_id].coll);

    int bcast_id = find_coll_description(mpi_coll_bcast_description, xbt_cfg_get_string("smpi/bcast"),"bcast");
    mpi_coll_bcast_fun = reinterpret_cast<int (*)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com)>
        (mpi_coll_bcast_description[bcast_id].coll);

    int reduce_id = find_coll_description(mpi_coll_reduce_description, xbt_cfg_get_string("smpi/reduce"),"reduce");
    mpi_coll_reduce_fun = reinterpret_cast<int (*)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)>
        (mpi_coll_reduce_description[reduce_id].coll);

    int reduce_scatter_id =
        find_coll_description(mpi_coll_reduce_scatter_description,
                              xbt_cfg_get_string("smpi/reduce-scatter"),"reduce_scatter");
    mpi_coll_reduce_scatter_fun = reinterpret_cast<int (*)(void *sbuf, void *rbuf, int *rcounts,MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)>
        (mpi_coll_reduce_scatter_description[reduce_scatter_id].coll);

    int scatter_id = find_coll_description(mpi_coll_scatter_description, xbt_cfg_get_string("smpi/scatter"),"scatter");
    mpi_coll_scatter_fun = reinterpret_cast<int (*)(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)>
        (mpi_coll_scatter_description[scatter_id].coll);

    int barrier_id = find_coll_description(mpi_coll_barrier_description, xbt_cfg_get_string("smpi/barrier"),"barrier");
    mpi_coll_barrier_fun = reinterpret_cast<int (*)(MPI_Comm comm)>
        (mpi_coll_barrier_description[barrier_id].coll);

    smpi_coll_cleanup_callback=nullptr;
    smpi_cpu_threshold = xbt_cfg_get_double("smpi/cpu-threshold");
    smpi_host_speed = xbt_cfg_get_double("smpi/host-speed");

    const char* smpi_privatize_option = xbt_cfg_get_string("smpi/privatize-global-variables");
    if (std::strcmp(smpi_privatize_option, "no") == 0)
      smpi_privatize_global_variables = SMPI_PRIVATIZE_NONE;
    else if (std::strcmp(smpi_privatize_option, "yes") == 0)
      smpi_privatize_global_variables = SMPI_PRIVATIZE_MMAP;
    else if (std::strcmp(smpi_privatize_option, "mmap") == 0)
      smpi_privatize_global_variables = SMPI_PRIVATIZE_MMAP;
    else if (std::strcmp(smpi_privatize_option, "dlopen") == 0)
      smpi_privatize_global_variables = SMPI_PRIVATIZE_DLOPEN;
    else
      xbt_die("Invalid value for smpi/privatize-global-variables: %s",
        smpi_privatize_option);

    if (smpi_cpu_threshold < 0)
      smpi_cpu_threshold = DBL_MAX;

    char* val = xbt_cfg_get_string("smpi/shared-malloc");
    if (!strcasecmp(val, "yes") || !strcmp(val, "1") || !strcasecmp(val, "on") || !strcasecmp(val, "global")) {
      smpi_cfg_shared_malloc = shmalloc_global;
    } else if (!strcasecmp(val, "local")) {
      smpi_cfg_shared_malloc = shmalloc_local;
    } else if (!strcasecmp(val, "no") || !strcmp(val, "0") || !strcasecmp(val, "off")) {
      smpi_cfg_shared_malloc = shmalloc_none;
    } else {
      xbt_die("Invalid value '%s' for option smpi/shared-malloc. Possible values: 'on' or 'global', 'local', 'off'",
              val);
    }
}

static int execute_command(const char * const argv[])
{
  pid_t pid;
  int status;
  if (posix_spawnp(&pid, argv[0], nullptr, nullptr, (char* const*) argv, environ) != 0)
    return 127;
  if (waitpid(status, &status, 0) != pid)
    return 127;
  return status;
}

typedef int (* smpi_entry_point_type)(int argc, char **argv);

static int smpi_run_entry_point(smpi_entry_point_type entry_point, std::vector<std::string> args)
{
  const int argc = args.size();
  std::unique_ptr<char*[]> argv(new char*[argc + 1]);
  for (int i = 0; i != argc; ++i)
    argv[i] = args[i].empty() ? const_cast<char*>(""): &args[i].front();
  argv[argc] = nullptr;

  int res = entry_point(argc, argv.get());
  if (res != 0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", res);
    smpi_process_data()->return_value = res;
  }
  return 0;
}

int smpi_main(const char* executable, int argc, char *argv[])
{
  srand(SMPI_RAND_SEED);

  if (getenv("SMPI_PRETEND_CC") != nullptr) {
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

  SMPI_switch_data_segment = &smpi_switch_data_segment;

  smpi_init_options();

  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);
  SIMIX_comm_set_copy_data_callback(&smpi_comm_copy_buffer_callback);

  static std::size_t rank = 0;

  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_DLOPEN) {

    std::string executable_copy = executable;
    simix_global->default_function = [executable_copy](std::vector<std::string> args) {
      return std::function<void()>([executable_copy, args] {

        // Copy the dynamic library:
        std::string target_executable = executable_copy
          + "_" + std::to_string(getpid())
          + "_" + std::to_string(rank++) + ".so";
        // TODO, execute directly instead of relying on cp
        const char* command1 [] = {
          "cp", "--reflink=auto", "--", executable_copy.c_str(), target_executable.c_str(),
          nullptr
        };
        const char* command2 [] = {
          "cp", "--", executable_copy.c_str(), target_executable.c_str(),
          nullptr
        };
        if (execute_command(command1) != 0 && execute_command(command2) != 0)
          xbt_die("copy failed");

        // Load the copy and resolve the entry point:
        void* handle = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL);
        unlink(target_executable.c_str());
        if (handle == nullptr)
          xbt_die("dlopen failed");
        smpi_entry_point_type entry_point = (int (*)(int, char**)) dlsym(handle, "main");
        // fprintf(stderr, "%s EP=%p\n", target_executable.c_str(), (void*) entry_point);
        if (entry_point == nullptr)
          xbt_die("Could not resolve entry point");

        smpi_run_entry_point(entry_point, args);
      });
    };

  }
  else {

    // Load the dynamic library and resolve the entry point:
    void* handle = dlopen(executable, RTLD_LAZY | RTLD_LOCAL);
    if (handle == nullptr)
      xbt_die("dlopen failed for %s", executable);
    smpi_entry_point_type entry_point = (int (*)(int, char**)) dlsym(handle, "main");
    if (entry_point == nullptr)
      xbt_die("main not found in %s", executable);
    // TODO, register the executable for SMPI privatization

    // Execute the same entry point for each simulated process:
    simix_global->default_function = [entry_point](std::vector<std::string> args) {
      return std::function<void()>([entry_point, args] {
        smpi_run_entry_point(entry_point, args);
      });
    };

  }

  SIMIX_launch_application(argv[2]);

  smpi_global_init();

  smpi_check_options();

  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP)
    smpi_initialize_global_memory_segments();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
  
    SIMIX_run();

    xbt_os_walltimer_stop(global_timer);
    if (xbt_cfg_get_boolean("smpi/display-timing")){
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
  int count = smpi_process_count();
  int i, ret=0;
  for (i = 0; i < count; i++) {
    if(process_data[i]->return_value!=0){
      ret=process_data[i]->return_value;//return first non 0 value
      break;
    }
  }
  smpi_global_destroy();

  TRACE_end();

  return ret;
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
  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP)
    smpi_initialize_global_memory_segments();
}

void SMPI_finalize(){
  smpi_global_destroy();
}
