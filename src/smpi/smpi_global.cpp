/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "private.h"
#include "private.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "smpi/smpi_shared_malloc.hpp"
#include "simgrid/sg_config.h"
#include "src/kernel/activity/SynchroComm.hpp"
#include "src/mc/mc_record.h"
#include "src/mc/mc_replay.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_private.h"
#include "surf/surf.h"
#include "xbt/replay.hpp"
#include <xbt/config.hpp>

#include <float.h> /* DBL_MAX */
#include <fstream>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

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

simgrid::smpi::Process **process_data = nullptr;
int process_count = 0;
int smpi_universe_size = 0;
int* index_to_process_data = nullptr;
extern double smpi_total_benched_time;
xbt_os_timer_t global_timer;
MPI_Comm MPI_COMM_WORLD = MPI_COMM_UNINITIALIZED;
MPI_Errhandler *MPI_ERRORS_RETURN = nullptr;
MPI_Errhandler *MPI_ERRORS_ARE_FATAL = nullptr;
MPI_Errhandler *MPI_ERRHANDLER_NULL = nullptr;
static simgrid::config::Flag<double> smpi_wtime_sleep(
  "smpi/wtime", "Minimum time to inject inside a call to MPI_Wtime", 0.0);
static simgrid::config::Flag<double> smpi_init_sleep(
  "smpi/init", "Time to inject inside a call to MPI_Init", 0.0);

void (*smpi_comm_copy_data_callback) (smx_activity_t, void*, size_t) = &smpi_comm_copy_buffer_callback;



int smpi_process_count()
{
  return process_count;
}

simgrid::smpi::Process* smpi_process()
{
  simgrid::MsgActorExt* msgExt = static_cast<simgrid::MsgActorExt*>(SIMIX_process_self()->data);
  return static_cast<simgrid::smpi::Process*>(msgExt->data);
}

simgrid::smpi::Process* smpi_process_remote(int index)
{
  return process_data[index_to_process_data[index]];
}

MPI_Comm smpi_process_comm_self(){
  return smpi_process()->comm_self();
}

void smpi_process_init(int *argc, char ***argv){
  simgrid::smpi::Process::init(argc, argv);
}

int smpi_process_index(){
  return smpi_process()->index();
}


int smpi_global_size()
{
  char *value = getenv("SMPI_GLOBAL_SIZE");
  xbt_assert(value,"Please set env var SMPI_GLOBAL_SIZE to the expected number of processes.");

  return xbt_str_parse_int(value, "SMPI_GLOBAL_SIZE contains a non-numerical value: %s");
}

void smpi_comm_set_copy_data_callback(void (*callback) (smx_activity_t, void*, size_t))
{
  smpi_comm_copy_data_callback = callback;
}

static void print(std::vector<std::pair<size_t, size_t>> vec) {
    fprintf(stderr, "{");
    for(auto elt: vec) {
        fprintf(stderr, "(0x%lx, 0x%lx),", elt.first, elt.second);
    }
    fprintf(stderr, "}\n");
}
static void memcpy_private(void *dest, const void *src, size_t n, std::vector<std::pair<size_t, size_t>> &private_blocks) {
  for(auto block : private_blocks) {
    memcpy((uint8_t*)dest+block.first, (uint8_t*)src+block.first, block.second-block.first);
  }
}

static void check_blocks(std::vector<std::pair<size_t, size_t>> &private_blocks, size_t buff_size) {
  for(auto block : private_blocks) {
    xbt_assert(block.first <= block.second && block.second <= buff_size, "Oops, bug in shared malloc.");
  }
}

void smpi_comm_copy_buffer_callback(smx_activity_t synchro, void *buff, size_t buff_size)
{
  simgrid::kernel::activity::Comm *comm = dynamic_cast<simgrid::kernel::activity::Comm*>(synchro);
  int src_shared=0, dst_shared=0;
  size_t src_offset=0, dst_offset=0;
  std::vector<std::pair<size_t, size_t>> src_private_blocks;
  std::vector<std::pair<size_t, size_t>> dst_private_blocks;
  XBT_DEBUG("Copy the data over");
  if((src_shared=smpi_is_shared(buff, src_private_blocks, &src_offset))) {
    XBT_DEBUG("Sender %p is shared. Let's ignore it.", buff);
    src_private_blocks = shift_and_frame_private_blocks(src_private_blocks, src_offset, buff_size);
  }
  else {
    src_private_blocks.clear();
    src_private_blocks.push_back(std::make_pair(0, buff_size));
  }
  if((dst_shared=smpi_is_shared((char*)comm->dst_buff, dst_private_blocks, &dst_offset))) {
    XBT_DEBUG("Receiver %p is shared. Let's ignore it.", (char*)comm->dst_buff);
    dst_private_blocks = shift_and_frame_private_blocks(dst_private_blocks, dst_offset, buff_size);
  }
  else {
    dst_private_blocks.clear();
    dst_private_blocks.push_back(std::make_pair(0, buff_size));
  }
/*
  fprintf(stderr, "size: 0x%x\n", buff_size);
  fprintf(stderr, "src: ");
  print(src_private_blocks);
  fprintf(stderr, "src_offset = 0x%x\n", src_offset);
  fprintf(stderr, "dst: ");
  print(dst_private_blocks);
  fprintf(stderr, "dst_offset = 0x%x\n", dst_offset);
*/
  check_blocks(src_private_blocks, buff_size);
  check_blocks(dst_private_blocks, buff_size);
  auto private_blocks = merge_private_blocks(src_private_blocks, dst_private_blocks);
/*
  fprintf(stderr, "Private blocks: ");
  print(private_blocks);
*/
  check_blocks(private_blocks, buff_size);
  void* tmpbuff=buff;
  if((smpi_privatize_global_variables) && (static_cast<char*>(buff) >= smpi_start_data_exe)
      && (static_cast<char*>(buff) < smpi_start_data_exe + smpi_size_data_exe )
    ){
       XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");

       smpi_switch_data_segment(
           (static_cast<simgrid::smpi::Process*>((static_cast<simgrid::MsgActorExt*>(comm->src_proc->data)->data))->index()));
       tmpbuff = static_cast<void*>(xbt_malloc(buff_size));
       memcpy_private(tmpbuff, buff, buff_size, private_blocks);
  }

  if((smpi_privatize_global_variables) && ((char*)comm->dst_buff >= smpi_start_data_exe)
      && ((char*)comm->dst_buff < smpi_start_data_exe + smpi_size_data_exe )){
       XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");
       smpi_switch_data_segment(
           (static_cast<simgrid::smpi::Process*>((static_cast<simgrid::MsgActorExt*>(comm->dst_proc->data)->data))->index()));
  }

  XBT_DEBUG("Copying %zu bytes from %p to %p", buff_size, tmpbuff,comm->dst_buff);
  memcpy_private(comm->dst_buff, tmpbuff, buff_size, private_blocks);

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
  process_data       = new simgrid::smpi::Process*[process_count];
  for (i = 0; i < process_count; i++) {
    process_data[i]                       = new simgrid::smpi::Process(i);
  }
  //if the process was launched through smpirun script we generate a global mpi_comm_world
  //if not, we let MPI_COMM_NULL, and the comm world will be private to each mpi instance
  if(smpirun){
    group = new  simgrid::smpi::Group(process_count);
    MPI_COMM_WORLD = new  simgrid::smpi::Comm(group, nullptr);
    MPI_Attr_put(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, reinterpret_cast<void *>(process_count));
    msg_bar_t bar = MSG_barrier_init(process_count);

    for (i = 0; i < process_count; i++) {
      group->set_mapping(i, i);
      process_data[i]->set_finalization_barrier(bar);
    }
  }
}

void smpi_global_destroy()
{
  int count = smpi_process_count();

  smpi_bench_destroy();
  smpi_shared_destroy();
  if (MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED){
      delete MPI_COMM_WORLD->group();
      MSG_barrier_destroy(process_data[0]->finalization_barrier());
  }else{
      smpi_deployment_cleanup_instances();
  }
  for (int i = 0; i < count; i++) {
    if(process_data[i]->comm_self()!=MPI_COMM_NULL){
      simgrid::smpi::Comm::destroy(process_data[i]->comm_self());
    }
    if(process_data[i]->comm_intra()!=MPI_COMM_NULL){
      simgrid::smpi::Comm::destroy(process_data[i]->comm_intra());
    }
    xbt_os_timer_free(process_data[i]->timer());
    xbt_mutex_destroy(process_data[i]->mailboxes_mutex());
    delete process_data[i];
  }
  delete[] process_data;
  process_data = nullptr;

  if (MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED){
    MPI_COMM_WORLD->cleanup_smp();
    MPI_COMM_WORLD->cleanup_attr<simgrid::smpi::Comm>();
    if(simgrid::smpi::Colls::smpi_coll_cleanup_callback!=nullptr)
      simgrid::smpi::Colls::smpi_coll_cleanup_callback();
    delete MPI_COMM_WORLD;
  }

  MPI_COMM_WORLD = MPI_COMM_NULL;

  if (!MC_is_active()) {
    xbt_os_timer_free(global_timer);
  }

  xbt_free(index_to_process_data);
  if(smpi_privatize_global_variables)
    smpi_destroy_global_memory_segments();
  smpi_free_static();
}

extern "C" {

#ifndef WIN32

void __attribute__ ((weak)) user_main_()
{
  xbt_die("Should not be in this smpi_simulated_main");
}

int __attribute__ ((weak)) smpi_simulated_main_(int argc, char **argv)
{
  simgrid::smpi::Process::init(&argc, &argv);
  user_main_();
  return 0;
}

inline static int smpi_main_wrapper(int argc, char **argv){
  int ret = smpi_simulated_main_(argc,argv);
  if(ret !=0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", ret);
    smpi_process()->set_return_value(ret);
  }
  return 0;
}

int __attribute__ ((weak)) main(int argc, char **argv)
{
  return smpi_main(smpi_main_wrapper, argc, argv);
}

#endif

static void smpi_init_logs(){

  /* Connect log categories.  See xbt/log.c */

  XBT_LOG_CONNECT(smpi);  /* Keep this line as soon as possible in this function: xbt_log_appender_file.c depends on it
                             DO NOT connect this in XBT or so, or it will be useless to xbt_log_appender_file.c */
  XBT_LOG_CONNECT(instr_smpi);
  XBT_LOG_CONNECT(smpi_bench);
  XBT_LOG_CONNECT(smpi_coll);
  XBT_LOG_CONNECT(smpi_colls);
  XBT_LOG_CONNECT(smpi_comm);
  XBT_LOG_CONNECT(smpi_datatype);
  XBT_LOG_CONNECT(smpi_dvfs);
  XBT_LOG_CONNECT(smpi_group);
  XBT_LOG_CONNECT(smpi_kernel);
  XBT_LOG_CONNECT(smpi_mpi);
  XBT_LOG_CONNECT(smpi_memory);
  XBT_LOG_CONNECT(smpi_op);
  XBT_LOG_CONNECT(smpi_pmpi);
  XBT_LOG_CONNECT(smpi_request);
  XBT_LOG_CONNECT(smpi_replay);
  XBT_LOG_CONNECT(smpi_rma);
  XBT_LOG_CONNECT(smpi_shared);
  XBT_LOG_CONNECT(smpi_utils);
}
}

static void smpi_init_options(){

    simgrid::smpi::Colls::set_collectives();
    simgrid::smpi::Colls::smpi_coll_cleanup_callback=nullptr;
    smpi_cpu_threshold = xbt_cfg_get_double("smpi/cpu-threshold");
    smpi_host_speed = xbt_cfg_get_double("smpi/host-speed");
    smpi_privatize_global_variables = xbt_cfg_get_boolean("smpi/privatize-global-variables");
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

int smpi_main(int (*realmain) (int argc, char *argv[]), int argc, char *argv[])
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
  SIMIX_comm_set_copy_data_callback(smpi_comm_copy_data_callback);
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
    if(process_data[i]->return_value()!=0){
      ret=process_data[i]->return_value();//return first non 0 value
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
  if(smpi_privatize_global_variables)
    smpi_initialize_global_memory_segments();
}

void SMPI_finalize(){
  smpi_global_destroy();
}

void smpi_mpi_init() {
  if(smpi_init_sleep > 0) 
    simcall_process_sleep(smpi_init_sleep);
}

double smpi_mpi_wtime(){
  double time;
  if (smpi_process()->initialized() != 0 && smpi_process()->finalized() == 0 && smpi_process()->sampling() == 0) {
    smpi_bench_end();
    time = SIMIX_get_clock();
    // to avoid deadlocks if used as a break condition, such as
    //     while (MPI_Wtime(...) < time_limit) {
    //       ....
    //     }
    // because the time will not normally advance when only calls to MPI_Wtime
    // are made -> deadlock (MPI_Wtime never reaches the time limit)
    if(smpi_wtime_sleep > 0) 
      simcall_process_sleep(smpi_wtime_sleep);
    smpi_bench_begin();
  } else {
    time = SIMIX_get_clock();
  }
  return time;
}

