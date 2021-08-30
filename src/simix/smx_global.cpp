/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/kernel/Timer.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/smpi/include/smpi_actor.hpp"

#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/xml/platf.hpp"

#include "simgrid/kernel/resource/Model.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/remote/AppSide.hpp"
#endif

#include <memory>

XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix, "Logging specific to SIMIX (kernel)");

std::unique_ptr<simgrid::simix::Global> simix_global;

namespace simgrid {
namespace simix {
config::Flag<bool> cfg_verbose_exit{"debug/verbose-exit", "Display the actor status at exit", true};

xbt_dynar_t simix_global_get_actors_addr()
{
#if SIMGRID_HAVE_MC
  return kernel::EngineImpl::get_instance()->get_actors_vector();
#else
  xbt_die("This function is intended to be used when compiling with MC");
#endif
}
xbt_dynar_t simix_global_get_dead_actors_addr()
{
#if SIMGRID_HAVE_MC
  return kernel::EngineImpl::get_instance()->get_dead_actors_vector();
#else
  xbt_die("This function is intended to be used when compiling with MC");
#endif
}

} // namespace simix
} // namespace simgrid

XBT_ATTRIB_NORETURN static void inthandler(int)
{
  if (simgrid::simix::cfg_verbose_exit) {
    XBT_INFO("CTRL-C pressed. The current status will be displayed before exit (disable that behavior with option "
             "'debug/verbose-exit').");
    simgrid::kernel::EngineImpl::get_instance()->display_all_actor_status();
  } else {
    XBT_INFO("CTRL-C pressed, exiting. Hiding the current process status since 'debug/verbose-exit' is set to false.");
  }
  exit(1);
}

#ifndef _WIN32
static void segvhandler(int signum, siginfo_t* siginfo, void* /*context*/)
{
  if ((siginfo->si_signo == SIGSEGV && siginfo->si_code == SEGV_ACCERR) || siginfo->si_signo == SIGBUS) {
    fprintf(stderr,
            "Access violation or Bus error detected.\n"
            "This probably comes from a programming error in your code, or from a stack\n"
            "overflow. If you are certain of your code, try increasing the stack size\n"
            "   --cfg=contexts/stack-size:XXX (current size is %u KiB).\n"
            "\n"
            "If it does not help, this may have one of the following causes:\n"
            "a bug in SimGrid, a bug in the OS or a bug in a third-party libraries.\n"
            "Failing hardware can sometimes generate such errors too.\n"
            "\n"
            "If you think you've found a bug in SimGrid, please report it along with a\n"
            "Minimal Working Example (MWE) reproducing your problem and a full backtrace\n"
            "of the fault captured with gdb or valgrind.\n",
            smx_context_stack_size / 1024);
  } else if (siginfo->si_signo == SIGSEGV) {
    fprintf(stderr, "Segmentation fault.\n");
#if HAVE_SMPI
    if (smpi_enabled() && smpi_cfg_privatization() == SmpiPrivStrategies::NONE) {
#if HAVE_PRIVATIZATION
      fprintf(stderr, "Try to enable SMPI variable privatization with --cfg=smpi/privatization:yes.\n");
#else
      fprintf(stderr, "Sadly, your system does not support --cfg=smpi/privatization:yes (yet).\n");
#endif /* HAVE_PRIVATIZATION */
    }
#endif /* HAVE_SMPI */
  }
  std::raise(signum);
}

/**
 * Install signal handler for SIGSEGV.  Check that nobody has already installed
 * its own handler.  For example, the Java VM does this.
 */
static void install_segvhandler()
{
  stack_t old_stack;

  if (simgrid::kernel::context::Context::install_sigsegv_stack(&old_stack, true) == -1) {
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
  action.sa_flags     = SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
  sigemptyset(&action.sa_mask);

  /* Linux tend to raise only SIGSEGV where other systems also raise SIGBUS on severe error */
  for (int sig : {SIGSEGV, SIGBUS}) {
    if (sigaction(sig, &action, &old_action) == -1) {
      XBT_WARN("Failed to register signal handler for signal %d: %s", sig, strerror(errno));
      continue;
    }
    if ((old_action.sa_flags & SA_SIGINFO) || old_action.sa_handler != SIG_DFL) {
      XBT_DEBUG("A signal handler was already installed for signal %d (%p). Restore it.", sig,
                (old_action.sa_flags & SA_SIGINFO) ? (void*)old_action.sa_sigaction : (void*)old_action.sa_handler);
      sigaction(sig, &old_action, nullptr);
    }
  }
}

#endif /* _WIN32 */

static simgrid::kernel::actor::ActorCode maestro_code;
void SIMIX_set_maestro(void (*code)(void*), void* data)
{
#ifdef _WIN32
  XBT_INFO("WARNING, SIMIX_set_maestro is believed to not work on windows. Please help us investigating this issue if "
           "you need that feature");
#endif
  maestro_code = std::bind(code, data);
}

void SIMIX_global_init(int* argc, char** argv)
{
  if (simix_global != nullptr)
    return;

  simix_global = std::make_unique<simgrid::simix::Global>();

#if SIMGRID_HAVE_MC
  // The communication initialization is done ASAP, as we need to get some init parameters from the MC for different
  // layers. But simix_global needs to be created, as we send the address of some of its fields to the MC that wants to
  // read them directly.
  simgrid::mc::AppSide::initialize();
#endif

  surf_init(argc, argv); /* Initialize SURF structures */

  SIMIX_context_mod_init();

  // Either create a new context with maestro or create
  // a context object with the current context maestro):
  simgrid::kernel::actor::create_maestro(maestro_code);

  /* Prepare to display some more info when dying on Ctrl-C pressing */
  std::signal(SIGINT, inthandler);

#ifndef _WIN32
  install_segvhandler();
#endif
  /* register a function to be called by SURF after the environment creation */
  sg_platf_init();
  simgrid::s4u::Engine::on_platform_created.connect(surf_presolve);

  if (simgrid::config::get_value<bool>("debug/clean-atexit"))
    atexit(SIMIX_clean);
}

/**
 * @ingroup SIMIX_API
 * @brief Clean the SIMIX simulation
 *
 * This functions remove the memory used by SIMIX
 */
void SIMIX_clean()
{
  static bool smx_cleaned = false;
  if (smx_cleaned)
    return; // to avoid double cleaning by java and C

  smx_cleaned = true;
  XBT_DEBUG("SIMIX_clean called. Simulation's over.");
  auto* engine = simgrid::kernel::EngineImpl::get_instance();
  if (engine->has_actors_to_run() && simgrid::s4u::Engine::get_clock() <= 0.0) {
    XBT_CRITICAL("   ");
    XBT_CRITICAL("The time is still 0, and you still have processes ready to run.");
    XBT_CRITICAL("It seems that you forgot to run the simulation that you setup.");
    xbt_die("Bailing out to avoid that stop-before-start madness. Please fix your code.");
  }

#if HAVE_SMPI
  if (not engine->get_actor_list().empty()) {
    if (smpi_process()->initialized()) {
      xbt_die("Process exited without calling MPI_Finalize - Killing simulation");
    } else {
      XBT_WARN("Process called exit when leaving - Skipping cleanups");
      return;
    }
  }
#endif

  /* Kill all processes (but maestro) */
  simix_global->get_maestro()->kill_all();
  engine->run_all_actors();
  engine->empty_trash();

  /* Let's free maestro now */
  simix_global->destroy_maestro();

  /* Finish context module and SURF */
  simix_global->destroy_context_factory();

  surf_exit();

  simix_global = nullptr;
}

/**
 * @ingroup SIMIX_API
 * @brief A clock (in second).
 *
 * @return Return the clock.
 */
double SIMIX_get_clock() // XBT_ATTRIB_DEPRECATED_v332
{
  return simgrid::s4u::Engine::get_clock();
}

void SIMIX_run() // XBT_ATTRIB_DEPRECATED_v332
{
  simgrid::kernel::EngineImpl::get_instance()->run();
}

int SIMIX_is_maestro()
{
  if (simix_global == nullptr) // SimDag
    return true;
  const simgrid::kernel::actor::ActorImpl* self = SIMIX_process_self();
  return self == nullptr || simix_global->is_maestro(self);
}
