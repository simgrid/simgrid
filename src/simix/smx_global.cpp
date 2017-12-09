/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <boost/heap/fibonacci_heap.hpp>
#include <functional>
#include <memory>

#include "src/internal_config.h"
#include <csignal> /* Signal handling */
#include <cstdlib>

#include <xbt/algorithm.hpp>
#include <xbt/functional.hpp>
#include <xbt/utility.hpp>

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"

#include "smx_private.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf.hpp"
#include "xbt/ex.h" /* ex_backtrace_display */

#include "mc/mc.h"
#include "simgrid/sg_config.h"
#include "src/mc/mc_replay.hpp"
#include "src/surf/StorageImpl.hpp"

#include "src/smpi/include/smpi_process.hpp"

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/SynchroIo.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/Client.hpp"
#include "src/mc/remote/mc_protocol.h"
#endif

#include "src/mc/mc_record.hpp"

#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif

XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix, "Logging specific to SIMIX (kernel)");

std::unique_ptr<simgrid::simix::Global> simix_global;

namespace {
typedef std::pair<double, smx_timer_t> TimerQelt;
boost::heap::fibonacci_heap<TimerQelt, boost::heap::compare<simgrid::xbt::HeapComparator<TimerQelt>>> simix_timers;
}

/** @brief Timer datatype */
class s_smx_timer_t {
  double date = 0.0;

public:
  decltype(simix_timers)::handle_type handle_;
  simgrid::xbt::Task<void()> callback;
  double getDate() { return date; }
  s_smx_timer_t(double date, simgrid::xbt::Task<void()> callback) : date(date), callback(std::move(callback)) {}
};

void (*SMPI_switch_data_segment)(int) = nullptr;

int _sg_do_verbose_exit = 1;
static void inthandler(int)
{
  if ( _sg_do_verbose_exit ) {
     XBT_INFO("CTRL-C pressed. The current status will be displayed before exit (disable that behavior with option 'verbose-exit').");
     SIMIX_display_process_status();
  }
  else {
     XBT_INFO("CTRL-C pressed, exiting. Hiding the current process status since 'verbose-exit' is set to false.");
  }
  exit(1);
}

#ifndef _WIN32
static void segvhandler(int signum, siginfo_t* siginfo, void* /*context*/)
{
  if (siginfo->si_signo == SIGSEGV && siginfo->si_code == SEGV_ACCERR) {
    fprintf(stderr, "Access violation detected.\n"
                    "This probably comes from a programming error in your code, or from a stack\n"
                    "overflow. If you are certain of your code, try increasing the stack size\n"
                    "   --cfg=contexts/stack-size=XXX (current size is %u KiB).\n"
                    "\n"
                    "If it does not help, this may have one of the following causes:\n"
                    "a bug in SimGrid, a bug in the OS or a bug in a third-party libraries.\n"
                    "Failing hardware can sometimes generate such errors too.\n"
                    "\n"
                    "If you think you've found a bug in SimGrid, please report it along with a\n"
                    "Minimal Working Example (MWE) reproducing your problem and a full backtrace\n"
                    "of the fault captured with gdb or valgrind.\n",
            smx_context_stack_size / 1024);
  } else  if (siginfo->si_signo == SIGSEGV) {
    fprintf(stderr, "Segmentation fault.\n");
#if HAVE_SMPI
    if (smpi_enabled() && smpi_privatize_global_variables == SMPI_PRIVATIZE_NONE) {
#if HAVE_PRIVATIZATION
      fprintf(stderr, "Try to enable SMPI variable privatization with --cfg=smpi/privatization:yes.\n");
#else
      fprintf(stderr, "Sadly, your system does not support --cfg=smpi/privatization:yes (yet).\n");
#endif /* HAVE_PRIVATIZATION */
    }
#endif /* HAVE_SMPI */
  }
  raise(signum);
}

char sigsegv_stack[SIGSTKSZ];   /* alternate stack for SIGSEGV handler */

/**
 * Install signal handler for SIGSEGV.  Check that nobody has already installed
 * its own handler.  For example, the Java VM does this.
 */
static void install_segvhandler()
{
  stack_t stack;
  stack_t old_stack;
  stack.ss_sp = sigsegv_stack;
  stack.ss_size = sizeof sigsegv_stack;
  stack.ss_flags = 0;

  if (sigaltstack(&stack, &old_stack) == -1) {
    XBT_WARN("Failed to register alternate signal stack: %s", strerror(errno));
    return;
  }
  if (not(old_stack.ss_flags & SS_DISABLE)) {
    XBT_DEBUG("An alternate stack was already installed (sp=%p, size=%zu, flags=%x). Restore it.", old_stack.ss_sp,
              old_stack.ss_size, (unsigned)old_stack.ss_flags);
    sigaltstack(&old_stack, nullptr);
  }

  struct sigaction action;
  struct sigaction old_action;
  action.sa_sigaction = &segvhandler;
  action.sa_flags = SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
  sigemptyset(&action.sa_mask);

  if (sigaction(SIGSEGV, &action, &old_action) == -1) {
    XBT_WARN("Failed to register signal handler for SIGSEGV: %s", strerror(errno));
    return;
  }
  if ((old_action.sa_flags & SA_SIGINFO) || old_action.sa_handler != SIG_DFL) {
    XBT_DEBUG("A signal handler was already installed for SIGSEGV (%p). Restore it.",
             (old_action.sa_flags & SA_SIGINFO) ? (void*)old_action.sa_sigaction : (void*)old_action.sa_handler);
    sigaction(SIGSEGV, &old_action, nullptr);
  }
}

#endif /* _WIN32 */

/********************************* SIMIX **************************************/
double SIMIX_timer_next()
{
  return simix_timers.empty() ? -1.0 : simix_timers.top().first;
}

static void kill_process(smx_actor_t process)
{
  SIMIX_process_kill(process, nullptr);
}

static std::function<void()> maestro_code;

namespace simgrid {
namespace simix {

simgrid::xbt::signal<void()> onDeadlock;

XBT_PUBLIC(void) set_maestro(std::function<void()> code)
{
  maestro_code = std::move(code);
}

}
}

void SIMIX_set_maestro(void (*code)(void*), void* data)
{
#ifdef _WIN32
  XBT_INFO("WARNING, SIMIX_set_maestro is believed to not work on windows. Please help us investigating this issue if you need that feature");
#endif
  maestro_code = std::bind(code, data);
}

/**
 * \ingroup SIMIX_API
 * \brief Initialize SIMIX internal data.
 *
 * \param argc Argc
 * \param argv Argv
 */
void SIMIX_global_init(int *argc, char **argv)
{
#if SIMGRID_HAVE_MC
  // The communication initialization is done ASAP.
  // We need to communicate  initialization of the different layers to the model-checker.
  simgrid::mc::Client::initialize();
#endif

  if (not simix_global) {
    simix_global = std::unique_ptr<simgrid::simix::Global>(new simgrid::simix::Global());
    simix_global->maestro_process = nullptr;
    simix_global->create_process_function = &SIMIX_process_create;
    simix_global->kill_process_function = &kill_process;
    simix_global->cleanup_process_function = &SIMIX_process_cleanup;
    simix_global->mutex = xbt_os_mutex_init();

    surf_init(argc, argv);      /* Initialize SURF structures */
    SIMIX_context_mod_init();

    // Either create a new context with maestro or create
    // a context object with the current context mestro):
    simgrid::simix::create_maestro(maestro_code);

    /* Prepare to display some more info when dying on Ctrl-C pressing */
    signal(SIGINT, inthandler);

#ifndef _WIN32
    install_segvhandler();
#endif
    /* register a function to be called by SURF after the environment creation */
    sg_platf_init();
    simgrid::s4u::onPlatformCreated.connect(SIMIX_post_create_environment);
    simgrid::s4u::Host::onCreation.connect([](simgrid::s4u::Host& host) {
      if (host.extension<simgrid::simix::Host>() == nullptr) // another callback to the same signal may have created it
        host.extension_set<simgrid::simix::Host>(new simgrid::simix::Host());
    });

    simgrid::surf::storageCreatedCallbacks.connect([](simgrid::surf::StorageImpl* storage) {
      sg_storage_t s = simgrid::s4u::Storage::byName(storage->getCname());
      xbt_assert(s != nullptr, "Storage not found for name %s", storage->getCname());
    });
  }

  if (xbt_cfg_get_boolean("clean-atexit"))
    atexit(SIMIX_clean);

  if (_sg_cfg_exit_asap)
    exit(0);
}

int smx_cleaned = 0;
/**
 * \ingroup SIMIX_API
 * \brief Clean the SIMIX simulation
 *
 * This functions remove the memory used by SIMIX
 */
void SIMIX_clean()
{
  if (smx_cleaned)
    return; // to avoid double cleaning by java and C

  smx_cleaned = 1;
  XBT_DEBUG("SIMIX_clean called. Simulation's over.");
  if (not simix_global->process_to_run.empty() && SIMIX_get_clock() <= 0.0) {
    XBT_CRITICAL("   ");
    XBT_CRITICAL("The time is still 0, and you still have processes ready to run.");
    XBT_CRITICAL("It seems that you forgot to run the simulation that you setup.");
    xbt_die("Bailing out to avoid that stop-before-start madness. Please fix your code.");
  }

#if HAVE_SMPI
  if (SIMIX_process_count()>0){
    if(smpi_process()->initialized()){
      xbt_die("Process exited without calling MPI_Finalize - Killing simulation");
    }else{
      XBT_WARN("Process called exit when leaving - Skipping cleanups");
      return;
    }
  }
#endif

  /* Kill all processes (but maestro) */
  SIMIX_process_killall(simix_global->maestro_process, 1);
  SIMIX_context_runall();
  SIMIX_process_empty_trash();

  /* Exit the SIMIX network module */
  SIMIX_mailbox_exit();

  while (not simix_timers.empty()) {
    delete simix_timers.top().second;
    simix_timers.pop();
  }
  /* Free the remaining data structures */
  simix_global->process_to_run.clear();
  simix_global->process_that_ran.clear();
  simix_global->process_to_destroy.clear();
  simix_global->process_list.clear();

  xbt_os_mutex_destroy(simix_global->mutex);
  simix_global->mutex = nullptr;
#if SIMGRID_HAVE_MC
  xbt_dynar_free(&simix_global->actors_vector);
  xbt_dynar_free(&simix_global->dead_actors_vector);
#endif

  /* Let's free maestro now */
  delete simix_global->maestro_process->context;
  simix_global->maestro_process->context = nullptr;
  delete simix_global->maestro_process;
  simix_global->maestro_process = nullptr;

  /* Finish context module and SURF */
  SIMIX_context_mod_exit();

  surf_exit();

  simix_global = nullptr;
}


/**
 * \ingroup SIMIX_API
 * \brief A clock (in second).
 *
 * \return Return the clock.
 */
double SIMIX_get_clock()
{
  if(MC_is_active() || MC_record_replay_is_active()){
    return MC_process_clock_get(SIMIX_process_self());
  }else{
    return surf_get_clock();
  }
}

/** Wake up all processes waiting for a Surf action to finish */
static void SIMIX_wake_processes()
{
  surf_action_t action;

  for (auto const& model : *all_existing_models) {
    XBT_DEBUG("Handling the processes whose action failed (if any)");
    while ((action = surf_model_extract_failed_action_set(model))) {
      XBT_DEBUG("   Handling Action %p",action);
      SIMIX_simcall_exit(static_cast<simgrid::kernel::activity::ActivityImpl*>(action->getData()));
    }
    XBT_DEBUG("Handling the processes whose action terminated normally (if any)");
    while ((action = surf_model_extract_done_action_set(model))) {
      XBT_DEBUG("   Handling Action %p",action);
      if (action->getData() == nullptr)
        XBT_DEBUG("probably vcpu's action %p, skip", action);
      else
        SIMIX_simcall_exit(static_cast<simgrid::kernel::activity::ActivityImpl*>(action->getData()));
    }
  }
}

/** Handle any pending timer */
static bool SIMIX_execute_timers()
{
  bool result = false;
  while (not simix_timers.empty() && SIMIX_get_clock() >= simix_timers.top().first) {
    result = true;
    // FIXME: make the timers being real callbacks
    // (i.e. provide dispatchers that read and expand the args)
    smx_timer_t timer = simix_timers.top().second;
    simix_timers.pop();
    try {
      timer->callback();
    } catch (...) {
      xbt_die("Exception thrown ouf of timer callback");
    }
    delete timer;
  }
  return result;
}

/** Execute all the tasks that are queued
 *
 *  e.g. `.then()` callbacks of futures.
 **/
static bool SIMIX_execute_tasks()
{
  xbt_assert(simix_global->tasksTemp.empty());

  if (simix_global->tasks.empty())
    return false;

  using std::swap;
  do {
    // We don't want the callbacks to modify the vector we are iterating over:
    swap(simix_global->tasks, simix_global->tasksTemp);

    // Execute all the queued tasks:
    for (auto& task : simix_global->tasksTemp)
      task();

    simix_global->tasksTemp.clear();
  } while (not simix_global->tasks.empty());

  return true;
}

/**
 * \ingroup SIMIX_API
 * \brief Run the main simulation loop.
 */
void SIMIX_run()
{
  if (not MC_record_path.empty()) {
    simgrid::mc::replay(MC_record_path);
    return;
  }

  double time = 0;

  do {
    XBT_DEBUG("New Schedule Round; size(queue)=%zu", simix_global->process_to_run.size());

    SIMIX_execute_tasks();

    while (not simix_global->process_to_run.empty()) {
      XBT_DEBUG("New Sub-Schedule Round; size(queue)=%zu", simix_global->process_to_run.size());

      /* Run all processes that are ready to run, possibly in parallel */
      SIMIX_process_runall();

      /* answer sequentially and in a fixed arbitrary order all the simcalls that were issued during that sub-round */

      /* WARNING, the order *must* be fixed or you'll jeopardize the simulation reproducibility (see RR-7653) */

      /* Here, the order is ok because:
       *
       *   Short proof: only maestro adds stuff to the process_to_run array, so the execution order of user contexts do
       *   not impact its order.
       *
       *   Long proof: processes remain sorted through an arbitrary (implicit, complex but fixed) order in all cases.
       *
       *   - if there is no kill during the simulation, processes remain sorted according by their PID.
       *     Rationale: This can be proved inductively.
       *        Assume that process_to_run is sorted at a beginning of one round (it is at round 0: the deployment file
       *        is parsed linearly).
       *        Let's show that it is still so at the end of this round.
       *        - if a process is added when being created, that's from maestro. It can be either at startup
       *          time (and then in PID order), or in response to a process_create simcall. Since simcalls are handled
       *          in arbitrary order (inductive hypothesis), we are fine.
       *        - If a process is added because it's getting killed, its subsequent actions shouldn't matter
       *        - If a process gets added to process_to_run because one of their blocking action constituting the meat
       *          of a simcall terminates, we're still good. Proof:
       *          - You are added from SIMIX_simcall_answer() only. When this function is called depends on the resource
       *            kind (network, cpu, disk, whatever), but the same arguments hold. Let's take communications as an
       *            example.
       *          - For communications, this function is called from SIMIX_comm_finish().
       *            This function itself don't mess with the order since simcalls are handled in FIFO order.
       *            The function is called:
       *            - before the comm starts (invalid parameters, or resource already dead or whatever).
       *              The order then trivial holds since maestro didn't interrupt its handling of the simcall yet
       *            - because the communication failed or were canceled after startup. In this case, it's called from
       *              the function we are in, by the chunk:
       *                       set = model->states.failed_action_set;
       *                       while ((synchro = extract(set)))
       *                          SIMIX_simcall_post((smx_synchro_t) synchro->data);
       *              This order is also fixed because it depends of the order in which the surf actions were
       *              added to the system, and only maestro can add stuff this way, through simcalls.
       *              We thus use the inductive hypothesis once again to conclude that the order in which synchros are
       *              poped out of the set does not depend on the user code's execution order.
       *            - because the communication terminated. In this case, synchros are served in the order given by
       *                       set = model->states.done_action_set;
       *                       while ((synchro = extract(set)))
       *                          SIMIX_simcall_post((smx_synchro_t) synchro->data);
       *              and the argument is very similar to the previous one.
       *            So, in any case, the orders of calls to SIMIX_comm_finish() do not depend on the order in which user
       *            processes are executed.
       *          So, in any cases, the orders of processes within process_to_run do not depend on the order in which
       *          user processes were executed previously.
       *     So, if there is no killing in the simulation, the simulation reproducibility is not jeopardized.
       *   - If there is some process killings, the order is changed by this decision that comes from user-land
       *     But this decision may not have been motivated by a situation that were different because the simulation is
       *     not reproducible.
       *     So, even the order change induced by the process killing is perfectly reproducible.
       *
       *   So science works, bitches [http://xkcd.com/54/].
       *
       *   We could sort the process_that_ran array completely so that we can describe the order in which simcalls are
       *   handled (like "according to the PID of issuer"), but it's not mandatory (order is fixed already even if
       *   unfriendly).
       *   That would thus be a pure waste of time.
       */

      for (smx_actor_t const& process : simix_global->process_that_ran) {
        if (process->simcall.call != SIMCALL_NONE) {
          SIMIX_simcall_handle(&process->simcall, 0);
        }
      }

      SIMIX_execute_tasks();
      do {
        SIMIX_wake_processes();
      } while (SIMIX_execute_tasks());

      /* If only daemon processes remain, cancel their actions, mark them to die and reschedule them */
      if (simix_global->process_list.size() == simix_global->daemons.size())
        for (auto const& dmon : simix_global->daemons) {
          XBT_DEBUG("Kill %s", dmon->getCname());
          SIMIX_process_kill(dmon, simix_global->maestro_process);
        }
    }

    time = SIMIX_timer_next();
    if (time > -1.0 || not simix_global->process_list.empty()) {
      XBT_DEBUG("Calling surf_solve");
      time = surf_solve(time);
      XBT_DEBUG("Moving time ahead : %g", time);
    }

    /* Notify all the hosts that have failed */
    /* FIXME: iterate through the list of failed host and mark each of them */
    /* as failed. On each host, signal all the running processes with host_fail */

    // Execute timers and tasks until there isn't anything to be done:
    bool again = false;
    do {
      again = SIMIX_execute_timers();
      if (SIMIX_execute_tasks())
        again = true;
      SIMIX_wake_processes();
    } while (again);

    /* Autorestart all process */
    for (auto const& host : host_that_restart) {
      XBT_INFO("Restart processes on host %s", host->getCname());
      SIMIX_host_autorestart(host);
    }
    host_that_restart.clear();

    /* Clean processes to destroy */
    SIMIX_process_empty_trash();

    XBT_DEBUG("### time %f, #processes %zu, #to_run %zu", time, simix_global->process_list.size(),
              simix_global->process_to_run.size());

    if (simix_global->process_to_run.empty() && not simix_global->process_list.empty())
      simgrid::simix::onDeadlock();

  } while (time > -1.0 || not simix_global->process_to_run.empty());

  if (not simix_global->process_list.empty()) {

    TRACE_end();

    XBT_CRITICAL("Oops ! Deadlock or code not perfectly clean.");
    SIMIX_display_process_status();
    simgrid::s4u::onDeadlock();
    xbt_abort();
  }
  simgrid::s4u::onSimulationEnd();
}

/**
 *   \brief Set the date to execute a function
 *
 * Set the date to execute the function on the surf.
 *   \param date Date to execute function
 *   \param callback Function to be executed
 *   \param arg Parameters of the function
 *
 */
smx_timer_t SIMIX_timer_set(double date, void (*callback)(void*), void *arg)
{
  smx_timer_t timer = new s_smx_timer_t(date, simgrid::xbt::makeTask([callback, arg]() { callback(arg); }));
  timer->handle_    = simix_timers.emplace(std::make_pair(date, timer));
  return timer;
}

smx_timer_t SIMIX_timer_set(double date, simgrid::xbt::Task<void()> callback)
{
  smx_timer_t timer = new s_smx_timer_t(date, std::move(callback));
  timer->handle_    = simix_timers.emplace(std::make_pair(date, timer));
  return timer;
}

/** @brief cancels a timer that was added earlier */
void SIMIX_timer_remove(smx_timer_t timer) {
  simix_timers.erase(timer->handle_);
  delete timer;
}

/** @brief Returns the date at which the timer will trigger (or 0 if nullptr timer) */
double SIMIX_timer_get_date(smx_timer_t timer) {
  return timer ? timer->getDate() : 0;
}

/**
 * \brief Registers a function to create a process.
 *
 * This function registers a function to be called
 * when a new process is created. The function has
 * to call SIMIX_process_create().
 * \param function create process function
 */
void SIMIX_function_register_process_create(smx_creation_func_t function)
{
  simix_global->create_process_function = function;
}

/**
 * \brief Registers a function to kill a process.
 *
 * This function registers a function to be called when a process is killed. The function has to call the
 * SIMIX_process_kill().
 *
 * \param function Kill process function
 */
void SIMIX_function_register_process_kill(void_pfn_smxprocess_t function)
{
  simix_global->kill_process_function = function;
}

/**
 * \brief Registers a function to cleanup a process.
 *
 * This function registers a user function to be called when a process ends properly.
 *
 * \param function cleanup process function
 */
void SIMIX_function_register_process_cleanup(void_pfn_smxprocess_t function)
{
  simix_global->cleanup_process_function = function;
}


void SIMIX_display_process_status()
{
  int nbprocess = simix_global->process_list.size();

  XBT_INFO("%d processes are still running, waiting for something.", nbprocess);
  /*  List the process and their state */
  XBT_INFO("Legend of the following listing: \"Process <pid> (<name>@<host>): <status>\"");
  for (auto const& kv : simix_global->process_list) {
    smx_actor_t process = kv.second;

    if (process->waiting_synchro) {

      const char* synchro_description = "unknown";

      if (boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(process->waiting_synchro) != nullptr)
        synchro_description = "execution";

      if (boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(process->waiting_synchro) != nullptr)
        synchro_description = "communication";

      if (boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(process->waiting_synchro) != nullptr)
        synchro_description = "sleeping";

      if (boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(process->waiting_synchro) != nullptr)
        synchro_description = "synchronization";

      if (boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(process->waiting_synchro) != nullptr)
        synchro_description = "I/O";

      XBT_INFO("Process %lu (%s@%s): waiting for %s synchro %p (%s) in state %d to finish", process->pid,
               process->getCname(), process->host->getCname(), synchro_description, process->waiting_synchro.get(),
               process->waiting_synchro->name.c_str(), (int)process->waiting_synchro->state);
    }
    else {
      XBT_INFO("Process %lu (%s@%s)", process->pid, process->getCname(), process->host->getCname());
    }
  }
}

int SIMIX_is_maestro()
{
  smx_actor_t self = SIMIX_process_self();
  return simix_global == nullptr /*SimDag*/ || self == nullptr || self == simix_global->maestro_process;
}
