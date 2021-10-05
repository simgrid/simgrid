/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/plugins/file_system.h"
#include "simgrid/s4u/Engine.hpp"
#include "smpi_coll.hpp"
#include "smpi_config.hpp"
#include "smpi_f2c.hpp"
#include "smpi_host.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/smpi/include/smpi_actor.hpp"
#include "xbt/config.hpp"
#include "xbt/file.hpp"

#include <algorithm>
#include <array>
#include <boost/algorithm/string.hpp> /* split */
#include <boost/tokenizer.hpp>
#include <cerrno>
#include <cinttypes>
#include <cstdint> /* intmax_t */
#include <cstring> /* strerror */
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
std::map</* computation unit name */ std::string, papi_process_data, std::less<>> units2papi_setup;
#endif

std::unordered_map<std::string, double> location2speedup;

static int smpi_exit_status = 0;
xbt_os_timer_t global_timer;
static std::vector<std::string> privatize_libs_paths;

// No instance gets manually created; check also the smpirun.in script as
// this default name is used there as well (when the <actor> tag is generated).
static const std::string smpi_default_instance_name("smpirun");

static simgrid::config::Flag<std::string>
    smpi_hostfile("smpi/hostfile",
                  "Classical MPI hostfile containing list of machines to dispatch "
                  "the processes, one per line",
                  "");

static simgrid::config::Flag<std::string> smpi_replay("smpi/replay",
                                                      "Replay a trace instead of executing the application", "");

static simgrid::config::Flag<int> smpi_np("smpi/np", "Number of processes to be created", 0);

static simgrid::config::Flag<int> smpi_map("smpi/map", "Display the mapping between nodes and processes", 0);

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
    saved_callback(comm, buff, size);
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
    if (src_private_blocks.empty()) { // simple shared malloc ... return.
      XBT_VERB("Sender is shared. Let's ignore it.");
      smpi_cleanup_comm_after_copy(comm, buff);
      return;
    }
  }
  else {
    src_private_blocks.clear();
    src_private_blocks.emplace_back(0, buff_size);
  }
  if (smpi_is_shared((char*)comm->dst_buff_, dst_private_blocks, &dst_offset)) {
    dst_private_blocks = shift_and_frame_private_blocks(dst_private_blocks, dst_offset, buff_size);
    if (dst_private_blocks.empty()) { // simple shared malloc ... return.
      XBT_VERB("Receiver is shared. Let's ignore it.");
      smpi_cleanup_comm_after_copy(comm, buff);
      return;
    }
  }
  else {
    dst_private_blocks.clear();
    dst_private_blocks.emplace_back(0, buff_size);
  }
  check_blocks(src_private_blocks, buff_size);
  check_blocks(dst_private_blocks, buff_size);
  auto private_blocks = merge_private_blocks(src_private_blocks, dst_private_blocks);
  check_blocks(private_blocks, buff_size);
  void* tmpbuff=buff;
  if (smpi_switch_data_segment(comm->src_actor_->get_iface(), buff)) {
    XBT_DEBUG("Privatization : We are copying from a zone inside global memory... Saving data to temp buffer !");
    tmpbuff = xbt_malloc(buff_size);
    memcpy_private(tmpbuff, buff, private_blocks);
  }

  if (smpi_switch_data_segment(comm->dst_actor_->get_iface(), comm->dst_buff_))
    XBT_DEBUG("Privatization : We are copying to a zone inside global memory - Switch data segment");

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

  if (smpi_cfg_papi_events_file().empty())
    return;

  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    XBT_ERROR("Could not initialize PAPI library; is it correctly installed and linked? Expected version is %u",
              PAPI_VER_CURRENT);
    return;
  }

  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
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
      break;
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
      auto* event_name = const_cast<char*>((*events_it).c_str());
      if (PAPI_event_name_to_code(event_name, &event_code) != PAPI_OK) {
        XBT_CRITICAL("Could not find PAPI event '%s'. Skipping.", event_name);
        continue;
      }
      if (PAPI_add_event(event_set, event_code) != PAPI_OK) {
        XBT_ERROR("Could not add PAPI event '%s'. Skipping.", event_name);
        continue;
      }
      XBT_DEBUG("Successfully added PAPI event '%s' to the event set.", event_name);

      counters2values.emplace_back(*events_it, 0LL);
    }

    std::string unit_name    = *(event_tokens.begin());
    papi_process_data config = {.counter_data = std::move(counters2values), .event_set = event_set};

    units2papi_setup.insert(std::make_pair(unit_name, std::move(config)));
  }
#endif
}

using smpi_entry_point_type         = std::function<int(int argc, char* argv[])>;
using smpi_c_entry_point_type       = int (*)(int argc, char** argv);
using smpi_fortran_entry_point_type = void (*)();

template <typename F>
static int smpi_run_entry_point(const F& entry_point, const std::string& executable_path,
                                const std::vector<std::string>& args)
{
  // copy C strings, we need them writable
  std::vector<char*> args4argv(args.size());
  std::transform(begin(args) + 1, end(args), begin(args4argv) + 1,
                 [](const std::string& s) { return xbt_strdup(s.c_str()); });

  // set argv[0] to executable_path
  args4argv[0] = xbt_strdup(executable_path.c_str());
  // add the final NULL
  args4argv.push_back(nullptr);

  // take a copy of args4argv to keep reference of the allocated strings
  const std::vector<char*> args2str(args4argv);

  try {
    int argc    = static_cast<int>(args4argv.size() - 1);
    char** argv = args4argv.data();
    int res = entry_point(argc, argv);
    if (res != 0) {
      XBT_WARN("SMPI process did not return 0. Return value : %d", res);
      if (smpi_exit_status == 0)
        smpi_exit_status = res;
    }
  } catch (simgrid::ForcefulKillException const& e) {
    XBT_DEBUG("Caught a ForcefulKillException: %s", e.what());
  }

  for (char* s : args2str)
    xbt_free(s);

  return 0;
}

static smpi_entry_point_type smpi_resolve_function(void* handle)
{
  auto* entry_point_fortran = reinterpret_cast<smpi_fortran_entry_point_type>(dlsym(handle, "user_main_"));
  if (entry_point_fortran != nullptr) {
    return [entry_point_fortran](int, char**) {
      entry_point_fortran();
      return 0;
    };
  }

  auto* entry_point = reinterpret_cast<smpi_c_entry_point_type>(dlsym(handle, "main"));
  if (entry_point != nullptr) {
    return entry_point;
  }

  return smpi_entry_point_type();
}

static void smpi_copy_file(const std::string& src, const std::string& target, off_t fdin_size)
{
  int fdin = open(src.c_str(), O_RDONLY);
  xbt_assert(fdin >= 0, "Cannot read from %s. Please make sure that the file exists and is executable.", src.c_str());
  xbt_assert(unlink(target.c_str()) == 0 || errno == ENOENT, "Failed to unlink file %s: %s", target.c_str(),
             strerror(errno));
  int fdout = open(target.c_str(), O_CREAT | O_RDWR | O_EXCL, S_IRWXU);
  xbt_assert(fdout >= 0, "Cannot write into %s: %s", target.c_str(), strerror(errno));

  XBT_DEBUG("Copy %" PRIdMAX " bytes into %s", static_cast<intmax_t>(fdin_size), target.c_str());
#if SG_HAVE_SENDFILE
  ssize_t sent_size = sendfile(fdout, fdin, nullptr, fdin_size);
  if (sent_size == fdin_size) {
    close(fdin);
    close(fdout);
    return;
  }
  xbt_assert(sent_size == -1 && errno == ENOSYS,
             "Error while copying %s: only %zd bytes copied instead of %" PRIdMAX " (errno: %d -- %s)", target.c_str(),
             sent_size, static_cast<intmax_t>(fdin_size), errno, strerror(errno));
#endif
  // If this point is reached, sendfile() actually is not available.  Copy file by hand.
  std::vector<unsigned char> buf(1024 * 1024 * 4);
  while (ssize_t got = read(fdin, buf.data(), buf.size())) {
    if (got == -1) {
      xbt_assert(errno == EINTR, "Cannot read from %s", src.c_str());
      continue;
    }
    const unsigned char* p = buf.data();
    ssize_t todo           = got;
    while (ssize_t done = write(fdout, p, todo)) {
      if (done == -1) {
        xbt_assert(errno == EINTR, "Cannot write into %s", target.c_str());
        continue;
      }
      p += done;
      todo -= done;
    }
  }
  close(fdin);
  close(fdout);
}

#if not defined(__APPLE__) && not defined(__HAIKU__)
static int visit_libs(struct dl_phdr_info* info, size_t, void* data)
{
  auto* libname    = static_cast<std::string*>(data);
  std::string path = info->dlpi_name;
  if (path.find(*libname) != std::string::npos) {
    *libname = std::move(path);
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
      xbt_assert(libhandle != nullptr, "Cannot dlopen %s - check your settings in smpi/privatize-libs",
                 libname.c_str());
      // get library name from path
      std::string fullpath = libname;
#if not defined(__APPLE__) && not defined(__HAIKU__)
      xbt_assert(dl_iterate_phdr(visit_libs, &fullpath) != 0,
                 "Can't find a linked %s - check your settings in smpi/privatize-libs", fullpath.c_str());
      XBT_DEBUG("Extra lib to privatize '%s' found", fullpath.c_str());
#else
      xbt_die("smpi/privatize-libs is not (yet) compatible with OSX nor with Haiku");
#endif
      privatize_libs_paths.emplace_back(std::move(fullpath));
      dlclose(libhandle);
    }
  }

  simgrid::s4u::Engine::get_instance()->register_default([executable, fdin_size](std::vector<std::string> args) {
    return simgrid::kernel::actor::ActorCode([executable, fdin_size, args = std::move(args)] {
      static std::size_t rank = 0;
      // Copy the dynamic library:
      simgrid::xbt::Path path(executable);
      std::string target_executable = simgrid::config::get_value<std::string>("smpi/tmpdir") + "/" +
          path.get_base_name() + "_" + std::to_string(getpid()) + "_" + std::to_string(rank) + ".so";

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
          auto pad                   = std::min<size_t>(7, libname.length());
          std::string target_libname = std::string(pad - std::to_string(rank).length(), '0') + std::to_string(rank) + libname.substr(pad);
          std::string target_lib = simgrid::config::get_value<std::string>("smpi/tmpdir") + "/" + target_libname;
          target_libs.push_back(target_lib);
          XBT_DEBUG("copy lib %s to %s, with size %lld", libpath.c_str(), target_lib.c_str(), (long long)fdin_size2);
          smpi_copy_file(libpath, target_lib, fdin_size2);

          std::string sedcommand = "sed -i -e 's/" + libname + "/" + target_libname + "/g' " + target_executable;
          int status             = system(sedcommand.c_str());
          xbt_assert(status == 0, "error while applying sed command %s \n", sedcommand.c_str());
        }
      }

      rank++;
      // Load the copy and resolve the entry point:
      void* handle    = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND);
      int saved_errno = errno;
      if (not simgrid::config::get_value<bool>("smpi/keep-temps")) {
        unlink(target_executable.c_str());
        for (const std::string& target_lib : target_libs)
          unlink(target_lib.c_str());
      }
      xbt_assert(handle != nullptr,
                 "dlopen failed: %s (errno: %d -- %s).\nError: Did you compile the program with a SMPI-specific "
                 "compiler (spmicc or friends)?",
                 dlerror(), saved_errno, strerror(saved_errno));

      smpi_entry_point_type entry_point = smpi_resolve_function(handle);
      xbt_assert(entry_point, "Could not resolve entry point. Does your program contain a main() function?");
      smpi_run_entry_point(entry_point, executable, args);
    });
  });
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
  simgrid::s4u::Engine::get_instance()->register_default([entry_point, executable](std::vector<std::string> args) {
    return simgrid::kernel::actor::ActorCode([entry_point, executable, args = std::move(args)] {
      if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP) {
        simgrid::smpi::ActorExt* ext = smpi_process();
        /* Now using the segment index of this process  */
        ext->set_privatized_region(smpi_init_global_memory_segment_process());
        /* Done at the process's creation */
        smpi_switch_data_segment(simgrid::s4u::Actor::self());
      }
      smpi_run_entry_point(entry_point, executable, args);
    });
  });
}

int smpi_main(const char* executable, int argc, char* argv[])
{
  if (getenv("SMPI_PRETEND_CC") != nullptr) {
    /* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the
     * configuration tools */
    return 0;
  }

  smpi_init_options_internal(true);
  simgrid::s4u::Engine engine(&argc, argv);

  sg_storage_file_system_init();
  // parse the platform file: get the host list
  engine.load_platform(argv[1]);
  engine.set_default_comm_data_copy_callback(smpi_comm_copy_buffer_callback);

  if (smpi_cfg_privatization() == SmpiPrivStrategies::DLOPEN)
    smpi_init_privatization_dlopen(executable);
  else
    smpi_init_privatization_no_dlopen(executable);

  simgrid::smpi::colls::set_collectives();
  simgrid::smpi::colls::smpi_coll_cleanup_callback = nullptr;

  std::vector<char*> args4argv(argv + 1, argv + argc + 1); // last element is NULL
  args4argv[0]     = xbt_strdup(executable);
  int real_argc    = argc - 1;
  char** real_argv = args4argv.data();

  // Setup argc/argv for the Fortran run-time environment
#if SMPI_IFORT
  for_rtl_init_(&real_argc, real_argv);
#elif SMPI_FLANG
  __io_set_argc(real_argc);
  __io_set_argv(real_argv);
#elif SMPI_GFORTRAN
  _gfortran_set_args(real_argc, real_argv);
#endif

  SMPI_init();

  const std::vector<const char*> args(real_argv + 1, real_argv + real_argc);
  int rank_counts =
      smpi_deployment_smpirun(&engine, smpi_hostfile.get(), smpi_np.get(), smpi_replay.get(), smpi_map.get(), args);

  SMPI_app_instance_register(smpi_default_instance_name.c_str(), nullptr, rank_counts);
  MPI_COMM_WORLD = *smpi_deployment_comm_world(smpi_default_instance_name);

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
    engine.get_impl()->run();

    xbt_os_walltimer_stop(global_timer);
    simgrid::smpi::utils::print_time_analysis(xbt_os_timer_elapsed(global_timer));
  }
  SMPI_finalize();

#if SMPI_IFORT
  for_rtl_finish_();
#endif
  xbt_free(args4argv[0]);

  return smpi_exit_status;
}

// Called either directly from the user code, or from the code called by smpirun
void SMPI_init(){
  smpi_init_options_internal(false);
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

  simgrid::smpi::utils::print_memory_analysis();
}

void smpi_mpi_init() {
  smpi_init_fortran_types();
  if(_smpi_init_sleep > 0)
    simgrid::s4u::this_actor::sleep_for(_smpi_init_sleep);
  if (not MC_is_active()) {
    smpi_deployment_startup_barrier(smpi_process()->get_instance_id());
  }
}

void SMPI_thread_create() {
  TRACE_smpi_init(simgrid::s4u::this_actor::get_pid(), __func__);
  smpi_process()->mark_as_initialized();
}

void smpi_exit(int res){
  if(res != 0){
    XBT_WARN("SMPI process did not return 0. Return value : %d", res);
    smpi_exit_status = res;
  }
  simgrid::s4u::this_actor::exit();
  THROW_IMPOSSIBLE;
}
