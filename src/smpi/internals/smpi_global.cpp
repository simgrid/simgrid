/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "SmpiHost.hpp"
#include "mc/mc.h"
#include "private.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_group.hpp"
#include "smpi_info.hpp"
#include "smpi_process.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/config.hpp"

#include <cfloat> /* DBL_MAX */
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>

#if HAVE_SENDFILE
#include <sys/sendfile.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi, "Logging specific to SMPI (kernel)");
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp> /* trim_right / trim_left */

#ifndef RTLD_DEEPBIND
/* RTLD_DEEPBIND is a bad idea of GNU ld that obviously does not exist on other platforms
 * See https://www.akkadia.org/drepper/dsohowto.pdf
 * and https://lists.freebsd.org/pipermail/freebsd-current/2016-March/060284.html
*/
#define RTLD_DEEPBIND 0
#endif

#if HAVE_PAPI
#include "papi.h"
const char* papi_default_config_name = "default";

struct papi_process_data {
  papi_counter_t counter_data;
  int event_set;
};

#endif
std::unordered_map<std::string, double> location2speedup;

static simgrid::smpi::Process** process_data = nullptr;
int process_count = 0;
int smpi_universe_size = 0;
int* index_to_process_data = nullptr;
extern double smpi_total_benched_time;
xbt_os_timer_t global_timer;
/**
 * Setting MPI_COMM_WORLD to MPI_COMM_UNINITIALIZED (it's a variable)
 * is important because the implementation of MPI_Comm checks
 * "this == MPI_COMM_UNINITIALIZED"? If yes, it uses smpi_process()->comm_world()
 * instead of "this".
 * This is basically how we only have one global variable but all processes have
 * different communicators (basically, the one their SMPI instance uses).
 *
 * See smpi_comm.cpp and the functions therein for details.
 */
MPI_Comm MPI_COMM_WORLD = MPI_COMM_UNINITIALIZED;
MPI_Errhandler *MPI_ERRORS_RETURN = nullptr;
MPI_Errhandler *MPI_ERRORS_ARE_FATAL = nullptr;
MPI_Errhandler *MPI_ERRHANDLER_NULL = nullptr;
// No instance gets manually created; check also the smpirun.in script as
// this default name is used there as well (when the <actor> tag is generated).
static const char* smpi_default_instance_name = "smpirun";
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
  smx_actor_t me = SIMIX_process_self();
  if (me == nullptr) // This happens sometimes (eg, when linking against NS3 because it pulls openMPI...)
    return nullptr;
  simgrid::msg::ActorExt* msgExt = static_cast<simgrid::msg::ActorExt*>(me->userdata);
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

void * smpi_process_get_user_data(){
  return smpi_process()->get_user_data();
}

void smpi_process_set_user_data(void *data){
  return smpi_process()->set_user_data(data);
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
  std::fprintf(stderr, "{");
  for (auto const& elt : vec) {
    std::fprintf(stderr, "(0x%zx, 0x%zx),", elt.first, elt.second);
  }
  std::fprintf(stderr, "}\n");
}
static void memcpy_private(void* dest, const void* src, std::vector<std::pair<size_t, size_t>>& private_blocks)
{
  for (auto const& block : private_blocks)
    memcpy((uint8_t*)dest+block.first, (uint8_t*)src+block.first, block.second-block.first);
}

static void check_blocks(std::vector<std::pair<size_t, size_t>> &private_blocks, size_t buff_size) {
  for (auto const& block : private_blocks)
    xbt_assert(block.first <= block.second && block.second <= buff_size, "Oops, bug in shared malloc.");
}

void smpi_comm_copy_buffer_callback(smx_activity_t synchro, void *buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
  int src_shared                        = 0;
  int dst_shared                        = 0;
  size_t src_offset                     = 0;
  size_t dst_offset                     = 0;
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
  check_blocks(src_private_blocks, buff_size);
  check_blocks(dst_private_blocks, buff_size);
  auto private_blocks = merge_private_blocks(src_private_blocks, dst_private_blocks);
  check_blocks(private_blocks, buff_size);
  void* tmpbuff=buff;
  if ((smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) && (static_cast<char*>(buff) >= smpi_data_exe_start) &&
      (static_cast<char*>(buff) < smpi_data_exe_start + smpi_data_exe_size)) {
    XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");

    smpi_switch_data_segment(
        static_cast<simgrid::smpi::Process*>((static_cast<simgrid::msg::ActorExt*>(comm->src_proc->userdata)->data))
            ->index());
    tmpbuff = static_cast<void*>(xbt_malloc(buff_size));
    memcpy_private(tmpbuff, buff, private_blocks);
  }

  if ((smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) && ((char*)comm->dst_buff >= smpi_data_exe_start) &&
      ((char*)comm->dst_buff < smpi_data_exe_start + smpi_data_exe_size)) {
    XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");
    smpi_switch_data_segment(
        static_cast<simgrid::smpi::Process*>((static_cast<simgrid::msg::ActorExt*>(comm->dst_proc->userdata)->data))
            ->index());
  }
  XBT_DEBUG("Copying %zu bytes from %p to %p", buff_size, tmpbuff,comm->dst_buff);
  memcpy_private(comm->dst_buff, tmpbuff, private_blocks);

  if (comm->detached) {
    // if this is a detached send, the source buffer was duplicated by SMPI
    // sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    //It seems that the request is used after the call there this should be free somewhere else but where???
    //xbt_free(comm->comm.src_data);// inside SMPI the request is kept inside the user data and should be free
    comm->src_buff = nullptr;
  }
  if (tmpbuff != buff)
    xbt_free(tmpbuff);
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
  if (not MC_is_active()) {
    global_timer = xbt_os_timer_new();
    xbt_os_walltimer_start(global_timer);
  }

  std::string filename = xbt_cfg_get_string("smpi/comp-adjustment-file");
  if (not filename.empty()) {
    std::ifstream fstream(filename);
    if (not fstream.is_open()) {
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

  if (not xbt_cfg_get_string("smpi/papi-events").empty()) {
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
      XBT_ERROR("Could not initialize PAPI library; is it correctly installed and linked?"
                " Expected version is %i",
                PAPI_VER_CURRENT);

    typedef boost::tokenizer<boost::char_separator<char>> Tokenizer;
    boost::char_separator<char> separator_units(";");
    std::string str = xbt_cfg_get_string("smpi/papi-events");
    Tokenizer tokens(str, separator_units);

    // Iterate over all the computational units. This could be processes, hosts, threads, ranks... You name it.
    // I'm not exactly sure what we will support eventually, so I'll leave it at the general term "units".
    for (auto const& unit_it : tokens) {
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
      for (Tokenizer::iterator events_it = ++(event_tokens.begin()); events_it != event_tokens.end(); ++events_it) {

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

  if (index_to_process_data == nullptr) {
    index_to_process_data = new int[SIMIX_process_count()];
  }

  bool smpirun = 0;
  if (process_count == 0) { // The program has been dispatched but no other
                            // SMPI instances have been registered. We're using smpirun.
    smpirun = true;
    SMPI_app_instance_register(smpi_default_instance_name, nullptr,
                               SIMIX_process_count()); // This call has a side effect on process_count...
    MPI_COMM_WORLD = *smpi_deployment_comm_world(smpi_default_instance_name);
  }
  smpi_universe_size = process_count;
  process_data       = new simgrid::smpi::Process*[process_count];
  for (int i = 0; i < process_count; i++) {
    if (smpirun) {
      process_data[i] = new simgrid::smpi::Process(i, smpi_deployment_finalization_barrier(smpi_default_instance_name));
      smpi_deployment_register_process(smpi_default_instance_name, i, i);
    } else {
      // TODO We can pass a nullptr here because Process::set_data() assigns the
      // barrier from the instance anyway. This is ugly and should be changed
      process_data[i] = new simgrid::smpi::Process(i, nullptr);
    }
  }
}

void smpi_global_destroy()
{
  smpi_bench_destroy();
  smpi_shared_destroy();
  smpi_deployment_cleanup_instances();
  int count = smpi_process_count();
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

  if (simgrid::smpi::Colls::smpi_coll_cleanup_callback != nullptr)
    simgrid::smpi::Colls::smpi_coll_cleanup_callback();

  MPI_COMM_WORLD = MPI_COMM_NULL;

  if (not MC_is_active()) {
    xbt_os_timer_free(global_timer);
  }

  delete[] index_to_process_data;
  if(smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP)
    smpi_destroy_global_memory_segments();
  smpi_free_static();
}

extern "C" {

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
  XBT_LOG_CONNECT(smpi_host);
  XBT_LOG_CONNECT(smpi_kernel);
  XBT_LOG_CONNECT(smpi_mpi);
  XBT_LOG_CONNECT(smpi_memory);
  XBT_LOG_CONNECT(smpi_op);
  XBT_LOG_CONNECT(smpi_pmpi);
  XBT_LOG_CONNECT(smpi_process);
  XBT_LOG_CONNECT(smpi_request);
  XBT_LOG_CONNECT(smpi_replay);
  XBT_LOG_CONNECT(smpi_rma);
  XBT_LOG_CONNECT(smpi_shared);
  XBT_LOG_CONNECT(smpi_utils);
}
}

static void smpi_init_options(){
  // return if already called
  if (smpi_cpu_threshold > -1)
    return;
  simgrid::smpi::Colls::set_collectives();
  simgrid::smpi::Colls::smpi_coll_cleanup_callback = nullptr;
  smpi_cpu_threshold                               = xbt_cfg_get_double("smpi/cpu-threshold");
  smpi_host_speed                                  = xbt_cfg_get_double("smpi/host-speed");
  std::string smpi_privatize_option                = xbt_cfg_get_string("smpi/privatization");
  if (smpi_privatize_option == "no" || smpi_privatize_option == "0")
    smpi_privatize_global_variables = SMPI_PRIVATIZE_NONE;
  else if (smpi_privatize_option == "yes" || smpi_privatize_option == "1")
    smpi_privatize_global_variables = SMPI_PRIVATIZE_DEFAULT;
  else if (smpi_privatize_option == "mmap")
    smpi_privatize_global_variables = SMPI_PRIVATIZE_MMAP;
  else if (smpi_privatize_option == "dlopen")
    smpi_privatize_global_variables = SMPI_PRIVATIZE_DLOPEN;
  else
    xbt_die("Invalid value for smpi/privatization: '%s'", smpi_privatize_option.c_str());

#if defined(__FreeBSD__)
    if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
      XBT_INFO("Mixing mmap privatization is broken on FreeBSD, switching to dlopen privatization instead.");
      smpi_privatize_global_variables = SMPI_PRIVATIZE_DLOPEN;
    }
#endif

    if (smpi_cpu_threshold < 0)
      smpi_cpu_threshold = DBL_MAX;

    std::string val = xbt_cfg_get_string("smpi/shared-malloc");
    if ((val == "yes") || (val == "1") || (val == "on") || (val == "global")) {
      smpi_cfg_shared_malloc = shmalloc_global;
    } else if (val == "local") {
      smpi_cfg_shared_malloc = shmalloc_local;
    } else if ((val == "no") || (val == "0") || (val == "off")) {
      smpi_cfg_shared_malloc = shmalloc_none;
    } else {
      xbt_die("Invalid value '%s' for option smpi/shared-malloc. Possible values: 'on' or 'global', 'local', 'off'",
              val.c_str());
    }
}

typedef std::function<int(int argc, char *argv[])> smpi_entry_point_type;
typedef int (* smpi_c_entry_point_type)(int argc, char **argv);
typedef void (*smpi_fortran_entry_point_type)();

static int smpi_run_entry_point(smpi_entry_point_type entry_point, std::vector<std::string> args)
{
  char noarg[]   = {'\0'};
  const int argc = args.size();
  std::unique_ptr<char*[]> argv(new char*[argc + 1]);
  for (int i = 0; i != argc; ++i)
    argv[i] = args[i].empty() ? noarg : &args[i].front();
  argv[argc] = nullptr;

  int res = entry_point(argc, argv.get());
  if (res != 0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", res);
    smpi_process()->set_return_value(res);
  }
  return 0;
}

// TODO, remove the number of functions involved here
static smpi_entry_point_type smpi_resolve_function(void* handle)
{
  smpi_fortran_entry_point_type entry_point_fortran = (smpi_fortran_entry_point_type)dlsym(handle, "user_main_");
  if (entry_point_fortran != nullptr) {
    return [entry_point_fortran](int argc, char** argv) {
      smpi_process_init(&argc, &argv);
      entry_point_fortran();
      return 0;
    };
  }

  smpi_c_entry_point_type entry_point = (smpi_c_entry_point_type)dlsym(handle, "main");
  if (entry_point != nullptr) {
    return entry_point;
  }

  return smpi_entry_point_type();
}

int smpi_main(const char* executable, int argc, char *argv[])
{
  srand(SMPI_RAND_SEED);

  if (getenv("SMPI_PRETEND_CC") != nullptr) {
    /* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the
     * configuration tools */
    return 0;
  }

  TRACE_global_init();

  SIMIX_global_init(&argc, argv);
  MSG_init(&argc,argv);

  SMPI_switch_data_segment = &smpi_switch_data_segment;

  simgrid::s4u::Host::onCreation.connect([](simgrid::s4u::Host& host) {
    host.extension_set(new simgrid::smpi::SmpiHost(&host));
  });

  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);
  SIMIX_comm_set_copy_data_callback(smpi_comm_copy_buffer_callback);

  smpi_init_options();

  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_DLOPEN) {

    std::string executable_copy = executable;

    // Prepare the copy of the binary (get its size)
    struct stat fdin_stat;
    stat(executable_copy.c_str(), &fdin_stat);
    off_t fdin_size = fdin_stat.st_size;
    static std::size_t rank = 0;

    simix_global->default_function = [executable_copy, fdin_size](std::vector<std::string> args) {
      return std::function<void()>([executable_copy, fdin_size, args] {

        // Copy the dynamic library:
        std::string target_executable = executable_copy
          + "_" + std::to_string(getpid())
          + "_" + std::to_string(rank++) + ".so";

        int fdin = open(executable_copy.c_str(), O_RDONLY);
        xbt_assert(fdin >= 0, "Cannot read from %s", executable_copy.c_str());
        int fdout = open(target_executable.c_str(), O_CREAT | O_RDWR, S_IRWXU);
        xbt_assert(fdout >= 0, "Cannot write into %s", target_executable.c_str());

#if HAVE_SENDFILE
        ssize_t sent_size = sendfile(fdout, fdin, NULL, fdin_size);
        xbt_assert(sent_size == fdin_size,
                   "Error while copying %s: only %zd bytes copied instead of %ld (errno: %d -- %s)",
                   target_executable.c_str(), sent_size, fdin_size, errno, strerror(errno));
#else
        XBT_VERB("Copy %d bytes into %s", static_cast<int>(fdin_size), target_executable.c_str());
        const int bufsize = 1024 * 1024 * 4;
        char buf[bufsize];
        while (int got = read(fdin, buf, bufsize)) {
          if (got == -1) {
            xbt_assert(errno == EINTR, "Cannot read from %s", executable_copy.c_str());
          } else {
            char* p  = buf;
            int todo = got;
            while (int done = write(fdout, p, todo)) {
              if (done == -1) {
                xbt_assert(errno == EINTR, "Cannot write into %s", target_executable.c_str());
              } else {
                p += done;
                todo -= done;
              }
            }
          }
        }
#endif
        close(fdin);
        close(fdout);

        // Load the copy and resolve the entry point:
        void* handle = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
        int saved_errno = errno;
        if (xbt_cfg_get_boolean("smpi/keep-temps") == false)
          unlink(target_executable.c_str());
        if (handle == nullptr)
          xbt_die("dlopen failed: %s (errno: %d -- %s)", dlerror(), saved_errno, strerror(saved_errno));
        smpi_entry_point_type entry_point = smpi_resolve_function(handle);
        if (not entry_point)
          xbt_die("Could not resolve entry point");

        smpi_run_entry_point(entry_point, args);
      });
    };

  }
  else {

    // Load the dynamic library and resolve the entry point:
    void* handle = dlopen(executable, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
    if (handle == nullptr)
      xbt_die("dlopen failed for %s: %s (errno: %d -- %s)", executable, dlerror(), errno, strerror(errno));
    smpi_entry_point_type entry_point = smpi_resolve_function(handle);
    if (not entry_point)
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

  SMPI_init();

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
  int ret   = 0;
  int count = smpi_process_count();
  for (int i = 0; i < count; i++) {
    if(process_data[i]->return_value()!=0){
      ret=process_data[i]->return_value();//return first non 0 value
      break;
    }
  }
  smpi_global_destroy();

  TRACE_end();

  return ret;
}

// Called either directly from the user code, or from the code called by smpirun
void SMPI_init(){
  smpi_init_logs();
  smpi_init_options();
  smpi_global_init();
  smpi_check_options();
  TRACE_smpi_alloc();
  simgrid::surf::surfExitCallbacks.connect(TRACE_smpi_release);
  if(smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP)
    smpi_backup_global_memory_segment();
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

