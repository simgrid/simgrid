/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/plugins/file_system.h"
#include "smpi_coll.hpp"
#include "smpi_f2c.hpp"
#include "smpi_host.hpp"
#include "smpi_config.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"
#include "xbt/config.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp> /* split */
#include <boost/tokenizer.hpp>
#include <cinttypes>
#include <cstdint> /* intmax_t */
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>

#if SG_HAVE_SENDFILE
#include <sys/sendfile.h>
#endif

#if HAVE_PAPI
#include "papi.h"
#endif

#if not defined(__APPLE__) && not defined(__HAIKU__)
#include <link.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi, "Logging specific to SMPI (kernel)");

#if SMPI_IFORT
  extern "C" void for_rtl_init_ (int *, char **);
  extern "C" void for_rtl_finish_ ();
#elif SMPI_FLANG
  extern "C" void __io_set_argc(int);
  extern "C" void __io_set_argv(char **);
#elif SMPI_GFORTRAN
  extern "C" void _gfortran_set_args(int, char **);
#endif

/* RTLD_DEEPBIND is a bad idea of GNU ld that obviously does not exist on other platforms
 * See https://www.akkadia.org/drepper/dsohowto.pdf
 * and https://lists.freebsd.org/pipermail/freebsd-current/2016-March/060284.html
*/
#if !RTLD_DEEPBIND || HAVE_SANITIZER_ADDRESS || HAVE_SANITIZER_THREAD
#define WANT_RTLD_DEEPBIND 0
#else
#define WANT_RTLD_DEEPBIND RTLD_DEEPBIND
#endif

#if HAVE_PAPI
std::string papi_default_config_name = "default";
std::map</* computation unit name */ std::string, papi_process_data> units2papi_setup;
#endif

std::unordered_map<std::string, double> location2speedup;

static int smpi_exit_status = 0;
extern double smpi_total_benched_time;
xbt_os_timer_t global_timer;
static std::vector<std::string> privatize_libs_paths;
/**
 * Setting MPI_COMM_WORLD to MPI_COMM_UNINITIALIZED (it's a variable)
 * is important because the implementation of MPI_Comm checks
 * "this == MPI_COMM_UNINITIALIZED"? If yes, it uses smpi_process()->comm_world()
 * instead of "this".
 * This is basically how we only have one global variable but all processes have
 * different communicators (the one their SMPI instance uses).
 *
 * See smpi_comm.cpp and the functions therein for details.
 */
MPI_Comm MPI_COMM_WORLD = MPI_COMM_UNINITIALIZED;
// No instance gets manually created; check also the smpirun.in script as
// this default name is used there as well (when the <actor> tag is generated).
static const std::string smpi_default_instance_name("smpirun");
static simgrid::config::Flag<double> smpi_init_sleep(
  "smpi/init", "Time to inject inside a call to MPI_Init", 0.0);

void (*smpi_comm_copy_data_callback)(simgrid::kernel::activity::CommImpl*, void*,
                                     size_t) = &smpi_comm_copy_buffer_callback;

simgrid::smpi::ActorExt* smpi_process()
{
  simgrid::s4u::ActorPtr me = simgrid::s4u::Actor::self();

  if (me == nullptr) // This happens sometimes (eg, when linking against NS3 because it pulls openMPI...)
    return nullptr;

  return me->extension<simgrid::smpi::ActorExt>();
}

simgrid::smpi::ActorExt* smpi_process_remote(simgrid::s4u::ActorPtr actor)
{
  if (actor.get() == nullptr)
    return nullptr;
  return actor->extension<simgrid::smpi::ActorExt>();
}

MPI_Comm smpi_process_comm_self(){
  return smpi_process()->comm_self();
}

MPI_Info smpi_process_info_env(){
  return smpi_process()->info_env();
}

void * smpi_process_get_user_data(){
  return simgrid::s4u::Actor::self()->get_data();
}

void smpi_process_set_user_data(void *data){
  simgrid::s4u::Actor::self()->set_data(data);
}

void smpi_comm_set_copy_data_callback(void (*callback) (smx_activity_t, void*, size_t))
{
  static void (*saved_callback)(smx_activity_t, void*, size_t);
  saved_callback               = callback;
  smpi_comm_copy_data_callback = [](simgrid::kernel::activity::CommImpl* comm, void* buff, size_t size) {
    saved_callback(smx_activity_t(comm), buff, size);
  };
}

static void memcpy_private(void* dest, const void* src, const std::vector<std::pair<size_t, size_t>>& private_blocks)
{
  for (auto const& block : private_blocks)
    memcpy((uint8_t*)dest+block.first, (uint8_t*)src+block.first, block.second-block.first);
}

static void check_blocks(const std::vector<std::pair<size_t, size_t>>& private_blocks, size_t buff_size)
{
  for (auto const& block : private_blocks)
    xbt_assert(block.first <= block.second && block.second <= buff_size, "Oops, bug in shared malloc.");
}

static void smpi_cleanup_comm_after_copy(simgrid::kernel::activity::CommImpl* comm, void* buff){
  if (comm->detached()) {
    // if this is a detached send, the source buffer was duplicated by SMPI
    // sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    //It seems that the request is used after the call there this should be free somewhere else but where???
    //xbt_free(comm->comm.src_data);// inside SMPI the request is kept inside the user data and should be free
    comm->src_buff_ = nullptr;
  }
}

void smpi_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  size_t src_offset                     = 0;
  size_t dst_offset                     = 0;
  std::vector<std::pair<size_t, size_t>> src_private_blocks;
  std::vector<std::pair<size_t, size_t>> dst_private_blocks;
  XBT_DEBUG("Copy the data over");
  if(smpi_is_shared(buff, src_private_blocks, &src_offset)) {
    src_private_blocks = shift_and_frame_private_blocks(src_private_blocks, src_offset, buff_size);
    if (src_private_blocks.size()==0){//simple shared malloc ... return.
      XBT_VERB("Sender is shared. Let's ignore it.");
      smpi_cleanup_comm_after_copy(comm, buff);
      return;
    }
  }
  else {
    src_private_blocks.clear();
    src_private_blocks.push_back(std::make_pair(0, buff_size));
  }
  if (smpi_is_shared((char*)comm->dst_buff_, dst_private_blocks, &dst_offset)) {
    dst_private_blocks = shift_and_frame_private_blocks(dst_private_blocks, dst_offset, buff_size);
    if (dst_private_blocks.size()==0){//simple shared malloc ... return.
      XBT_VERB("Receiver is shared. Let's ignore it.");
      smpi_cleanup_comm_after_copy(comm, buff);
      return;
    }
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
  if ((smpi_cfg_privatization() == SmpiPrivStrategies::MMAP) &&
      (static_cast<char*>(buff) >= smpi_data_exe_start) &&
      (static_cast<char*>(buff) < smpi_data_exe_start + smpi_data_exe_size)) {
    XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");
    smpi_switch_data_segment(comm->src_actor_->iface());
    tmpbuff = static_cast<void*>(xbt_malloc(buff_size));
    memcpy_private(tmpbuff, buff, private_blocks);
  }

  if ((smpi_cfg_privatization() == SmpiPrivStrategies::MMAP) &&
      ((char*)comm->dst_buff_ >= smpi_data_exe_start) &&
      ((char*)comm->dst_buff_ < smpi_data_exe_start + smpi_data_exe_size)) {
    XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");
    smpi_switch_data_segment(comm->dst_actor_->iface());
  }
  XBT_DEBUG("Copying %zu bytes from %p to %p", buff_size, tmpbuff, comm->dst_buff_);
  memcpy_private(comm->dst_buff_, tmpbuff, private_blocks);

  smpi_cleanup_comm_after_copy(comm,buff);
  if (tmpbuff != buff)
    xbt_free(tmpbuff);
}

void smpi_comm_null_copy_buffer_callback(simgrid::kernel::activity::CommImpl*, void*, size_t)
{
  /* nothing done in this version */
}

int smpi_enabled() {
  return MPI_COMM_WORLD != MPI_COMM_UNINITIALIZED;
}

static void smpi_init_papi()
{
#if HAVE_PAPI
  // This map holds for each computation unit (such as "default" or "process1" etc.)
  // the configuration as given by the user (counter data as a pair of (counter_name, counter_counter))
  // and the (computed) event_set.

  if (not smpi_cfg_papi_events_file().empty()) {
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
      XBT_ERROR("Could not initialize PAPI library; is it correctly installed and linked?"
                " Expected version is %u", PAPI_VER_CURRENT);

    typedef boost::tokenizer<boost::char_separator<char>> Tokenizer;
    boost::char_separator<char> separator_units(";");
    std::string str = smpi_cfg_papi_events_file();
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
      // This is important for PAPI: We need to map the values of counters back to the event_names (so, when PAPI_read()
      // has finished)!
      papi_counter_t counters2values;

      // Iterate over all counters that were specified for this specific unit.
      // Note that we need to remove the name of the unit (that could also be the "default" value), which always comes
      // first. Hence, we start at ++(events.begin())!
      for (Tokenizer::iterator events_it = ++(event_tokens.begin()); events_it != event_tokens.end(); ++events_it) {
        int event_code   = PAPI_NULL;
        char* event_name = const_cast<char*>((*events_it).c_str());
        if (PAPI_event_name_to_code(event_name, &event_code) != PAPI_OK) {
          XBT_CRITICAL("Could not find PAPI event '%s'. Skipping.", event_name);
          continue;
        }
        if (PAPI_add_event(event_set, event_code) != PAPI_OK) {
          XBT_ERROR("Could not add PAPI event '%s'. Skipping.", event_name);
          continue;
        }
        XBT_DEBUG("Successfully added PAPI event '%s' to the event set.", event_name);

        counters2values.push_back(
            // We cannot just pass *events_it, as this is of type const basic_string
            std::make_pair(std::string(*events_it), 0LL));
      }

      std::string unit_name    = *(event_tokens.begin());
      papi_process_data config = {.counter_data = std::move(counters2values), .event_set = event_set};

      units2papi_setup.insert(std::make_pair(unit_name, std::move(config)));
    }
  }
#endif
}



typedef std::function<int(int argc, char *argv[])> smpi_entry_point_type;
typedef int (* smpi_c_entry_point_type)(int argc, char **argv);
typedef void (*smpi_fortran_entry_point_type)();

template <typename F>
static int smpi_run_entry_point(const F& entry_point, const std::string& executable_path, std::vector<std::string> args)
{
  // copy C strings, we need them writable
  std::vector<char*>* args4argv = new std::vector<char*>(args.size());
  std::transform(begin(args), end(args), begin(*args4argv), [](const std::string& s) { return xbt_strdup(s.c_str()); });

  // set argv[0] to executable_path
  xbt_free((*args4argv)[0]);
  (*args4argv)[0] = xbt_strdup(executable_path.c_str());

#if !SMPI_IFORT
  // take a copy of args4argv to keep reference of the allocated strings
  const std::vector<char*> args2str(*args4argv);
#endif
  int argc = args4argv->size();
  args4argv->push_back(nullptr);
  char** argv = args4argv->data();

#if SMPI_IFORT
  for_rtl_init_ (&argc, argv);
#elif SMPI_FLANG
  __io_set_argc(argc);
  __io_set_argv(argv);
#elif SMPI_GFORTRAN
  _gfortran_set_args(argc, argv);
#endif 
  int res = entry_point(argc, argv);

#if SMPI_IFORT
  for_rtl_finish_ ();
#else
  for (char* s : args2str)
    xbt_free(s);
  delete args4argv;
#endif

  if (res != 0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", res);
    if (smpi_exit_status == 0)
      smpi_exit_status = res;
  }
  return 0;
}


// TODO, remove the number of functions involved here
static smpi_entry_point_type smpi_resolve_function(void* handle)
{
  smpi_fortran_entry_point_type entry_point_fortran = (smpi_fortran_entry_point_type)dlsym(handle, "user_main_");
  if (entry_point_fortran != nullptr) {
    return [entry_point_fortran](int, char**) {
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

static void smpi_copy_file(const std::string& src, const std::string& target, off_t fdin_size)
{
  int fdin = open(src.c_str(), O_RDONLY);
  xbt_assert(fdin >= 0, "Cannot read from %s. Please make sure that the file exists and is executable.", src.c_str());
  int fdout = open(target.c_str(), O_CREAT | O_RDWR, S_IRWXU);
  xbt_assert(fdout >= 0, "Cannot write into %s", target.c_str());

  XBT_DEBUG("Copy %" PRIdMAX " bytes into %s", static_cast<intmax_t>(fdin_size), target.c_str());
#if SG_HAVE_SENDFILE
  ssize_t sent_size = sendfile(fdout, fdin, NULL, fdin_size);
  if (sent_size == fdin_size) {
    close(fdin);
    close(fdout);
    return;
  } else if (sent_size != -1 || errno != ENOSYS) {
    xbt_die("Error while copying %s: only %zd bytes copied instead of %" PRIdMAX " (errno: %d -- %s)", target.c_str(),
            sent_size, static_cast<intmax_t>(fdin_size), errno, strerror(errno));
  }
#endif
  // If this point is reached, sendfile() actually is not available.  Copy file by hand.
  const int bufsize = 1024 * 1024 * 4;
  char* buf         = new char[bufsize];
  while (int got = read(fdin, buf, bufsize)) {
    if (got == -1) {
      xbt_assert(errno == EINTR, "Cannot read from %s", src.c_str());
    } else {
      const char* p = buf;
      int todo = got;
      while (int done = write(fdout, p, todo)) {
        if (done == -1) {
          xbt_assert(errno == EINTR, "Cannot write into %s", target.c_str());
        } else {
          p += done;
          todo -= done;
        }
      }
    }
  }
  delete[] buf;
  close(fdin);
  close(fdout);
}

#if not defined(__APPLE__) && not defined(__HAIKU__)
static int visit_libs(struct dl_phdr_info* info, size_t, void* data)
{
  char* libname = (char*)(data);
  const char *path = info->dlpi_name;
  if(strstr(path, libname)){
    strncpy(libname, path, 512);
    return 1;
  }
  
  return 0;
}
#endif

static void smpi_init_privatization_dlopen(const std::string& executable)
{
  // Prepare the copy of the binary (get its size)
  struct stat fdin_stat;
  stat(executable.c_str(), &fdin_stat);
  off_t fdin_size         = fdin_stat.st_size;

  std::string libnames = simgrid::config::get_value<std::string>("smpi/privatize-libs");
  if (not libnames.empty()) {
    // split option
    std::vector<std::string> privatize_libs;
    boost::split(privatize_libs, libnames, boost::is_any_of(";"));

    for (auto const& libname : privatize_libs) {
      // load the library once to add it to the local libs, to get the absolute path
      void* libhandle = dlopen(libname.c_str(), RTLD_LAZY);
      // get library name from path
      char fullpath[512] = {'\0'};
      strncpy(fullpath, libname.c_str(), 511);
#if not defined(__APPLE__) && not defined(__HAIKU__)
      xbt_assert(0 != dl_iterate_phdr(visit_libs, fullpath),
                 "Can't find a linked %s - check your settings in smpi/privatize-libs", fullpath);
      XBT_DEBUG("Extra lib to privatize '%s' found", fullpath);
#else
      xbt_die("smpi/privatize-libs is not (yet) compatible with OSX nor with Haiku");
#endif
      privatize_libs_paths.push_back(fullpath);
      dlclose(libhandle);
    }
  }

  simix_global->default_function = [executable, fdin_size](std::vector<std::string> args) {
    return std::function<void()>([executable, fdin_size, args] {
      static std::size_t rank = 0;
      // Copy the dynamic library:
      std::string target_executable =
          executable + "_" + std::to_string(getpid()) + "_" + std::to_string(rank) + ".so";

      smpi_copy_file(executable, target_executable, fdin_size);
      // if smpi/privatize-libs is set, duplicate pointed lib and link each executable copy to a different one.
      std::vector<std::string> target_libs;
      for (auto const& libpath : privatize_libs_paths) {
        // if we were given a full path, strip it
        size_t index = libpath.find_last_of("/\\");
        std::string libname;
        if (index != std::string::npos)
          libname = libpath.substr(index + 1);

        if (not libname.empty()) {
          // load the library to add it to the local libs, to get the absolute path
          struct stat fdin_stat2;
          stat(libpath.c_str(), &fdin_stat2);
          off_t fdin_size2 = fdin_stat2.st_size;

          // Copy the dynamic library, the new name must be the same length as the old one
          // just replace the name with 7 digits for the rank and the rest of the name.
          unsigned int pad = 7;
          if (libname.length() < pad)
            pad = libname.length();
          std::string target_lib =
              std::string(pad - std::to_string(rank).length(), '0') + std::to_string(rank) + libname.substr(pad);
          target_libs.push_back(target_lib);
          XBT_DEBUG("copy lib %s to %s, with size %lld", libpath.c_str(), target_lib.c_str(), (long long)fdin_size2);
          smpi_copy_file(libpath, target_lib, fdin_size2);

          std::string sedcommand = "sed -i -e 's/" + libname + "/" + target_lib + "/g' " + target_executable;
          xbt_assert(system(sedcommand.c_str()) == 0, "error while applying sed command %s \n", sedcommand.c_str());
        }
      }

      rank++;
      // Load the copy and resolve the entry point:
      void* handle    = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND);
      int saved_errno = errno;
      if (simgrid::config::get_value<bool>("smpi/keep-temps") == false) {
        unlink(target_executable.c_str());
        for (const std::string& target_lib : target_libs)
          unlink(target_lib.c_str());
      }
      xbt_assert(handle != nullptr, "dlopen failed: %s (errno: %d -- %s)", dlerror(), saved_errno,
                 strerror(saved_errno));

      smpi_entry_point_type entry_point = smpi_resolve_function(handle);
      xbt_assert(entry_point, "Could not resolve entry point");
      smpi_run_entry_point(entry_point, executable, args);
    });
  };
}

static void smpi_init_privatization_no_dlopen(const std::string& executable)
{
  if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP)
    smpi_prepare_global_memory_segment();

  // Load the dynamic library and resolve the entry point:
  void* handle = dlopen(executable.c_str(), RTLD_LAZY | RTLD_LOCAL);
  xbt_assert(handle != nullptr, "dlopen failed for %s: %s (errno: %d -- %s)", executable.c_str(), dlerror(), errno,
             strerror(errno));
  smpi_entry_point_type entry_point = smpi_resolve_function(handle);
  xbt_assert(entry_point, "main not found in %s", executable.c_str());

  if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP)
    smpi_backup_global_memory_segment();

  // Execute the same entry point for each simulated process:
  simix_global->default_function = [entry_point, executable](std::vector<std::string> args) {
    return std::function<void()>(
        [entry_point, executable, args] { smpi_run_entry_point(entry_point, executable, args); });
  };
}

int smpi_main(const char* executable, int argc, char* argv[])
{
  if (getenv("SMPI_PRETEND_CC") != nullptr) {
    /* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the
     * configuration tools */
    return 0;
  }
  
  SMPI_switch_data_segment = &smpi_switch_data_segment;
  smpi_init_options();
  TRACE_global_init();
  SIMIX_global_init(&argc, argv);

  auto engine              = simgrid::s4u::Engine::get_instance();

  sg_storage_file_system_init();
  // parse the platform file: get the host list
  engine->load_platform(argv[1]);
  SIMIX_comm_set_copy_data_callback(smpi_comm_copy_buffer_callback);

  if (smpi_cfg_privatization() == SmpiPrivStrategies::DLOPEN)
    smpi_init_privatization_dlopen(executable);
  else
    smpi_init_privatization_no_dlopen(executable);

  simgrid::smpi::colls::set_collectives();
  simgrid::smpi::colls::smpi_coll_cleanup_callback = nullptr;
  
  SMPI_init();

  /* This is a ... heavy way to count the MPI ranks */
  int rank_counts = 0;
  simgrid::s4u::Actor::on_creation.connect([&rank_counts](const simgrid::s4u::Actor& actor) {
    if (not actor.is_daemon())
      rank_counts++;
  });
  engine->load_deployment(argv[2]);

  SMPI_app_instance_register(smpi_default_instance_name.c_str(), nullptr, rank_counts);
  MPI_COMM_WORLD = *smpi_deployment_comm_world(smpi_default_instance_name);

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();

    xbt_os_walltimer_stop(global_timer);
    if (simgrid::config::get_value<bool>("smpi/display-timing")) {
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
  SMPI_finalize();

  return smpi_exit_status;
}

// Called either directly from the user code, or from the code called by smpirun
void SMPI_init(){
  smpi_init_options();
  simgrid::s4u::Actor::on_creation.connect([](simgrid::s4u::Actor& actor) {
    if (not actor.is_daemon())
      actor.extension_set<simgrid::smpi::ActorExt>(new simgrid::smpi::ActorExt(&actor));
  });
  simgrid::s4u::Host::on_creation.connect(
      [](simgrid::s4u::Host& host) { host.extension_set(new simgrid::smpi::Host(&host)); });
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts())
    host->extension_set(new simgrid::smpi::Host(host));

  if (not MC_is_active()) {
    global_timer = xbt_os_timer_new();
    xbt_os_walltimer_start(global_timer);
  }
  smpi_init_papi();
  smpi_check_options();
}

void SMPI_finalize()
{
  smpi_bench_destroy();
  smpi_shared_destroy();
  smpi_deployment_cleanup_instances();

  if (simgrid::smpi::colls::smpi_coll_cleanup_callback != nullptr)
    simgrid::smpi::colls::smpi_coll_cleanup_callback();

  MPI_COMM_WORLD = MPI_COMM_NULL;

  if (not MC_is_active()) {
    xbt_os_timer_free(global_timer);
  }

  if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP)
    smpi_destroy_global_memory_segments();
  if (simgrid::smpi::F2C::lookup() != nullptr)
    simgrid::smpi::F2C::delete_lookup();
}

void smpi_mpi_init() {
  smpi_init_fortran_types();
  if(smpi_init_sleep > 0)
    simgrid::s4u::this_actor::sleep_for(smpi_init_sleep);
}
