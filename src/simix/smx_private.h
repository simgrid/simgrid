/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PRIVATE_H
#define _SIMIX_PRIVATE_H

#include <functional>

#include "src/internal_config.h"
#include "simgrid/simix.h"
#include "surf/surf.h"
#include "xbt/base.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"
#include "xbt/config.h"
#include "xbt/xbt_os_time.h"
#include "xbt/function_types.h"
#include "src/xbt/ex_interface.h"
#include "src/instr/instr_private.h"
#include "smx_process_private.h"
#include "smx_host_private.h"
#include "smx_io_private.h"
#include "smx_network_private.h"
#include "popping_private.h"
#include "smx_synchro_private.h"

#include <signal.h>

#ifdef __cplusplus

#include <simgrid/simix.hpp>

namespace simgrid {
namespace simix {

/* This allows Java to hijack the context factory (Java induces factories of factory :) */
typedef ContextFactory* (*ContextFactoryInitializer)(void);
XBT_PUBLIC_DATA(ContextFactoryInitializer) factory_initializer;

XBT_PRIVATE ContextFactory* thread_factory();
XBT_PRIVATE ContextFactory* sysv_factory();
XBT_PRIVATE ContextFactory* raw_factory();
XBT_PRIVATE ContextFactory* boost_factory();

}
}

typedef simgrid::simix::ContextFactory *smx_context_factory_t;

#else

typedef struct s_smx_context_factory *smx_context_factory_t;

#endif

SG_BEGIN_DECL()

/********************************** Simix Global ******************************/
typedef struct s_smx_global {
  smx_context_factory_t context_factory;
  xbt_dynar_t process_to_run;
  xbt_dynar_t process_that_ran;
  xbt_swag_t process_list;
  xbt_swag_t process_to_destroy;
  smx_process_t maestro_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_pfn_smxprocess_t kill_process_function;
  /** Callback used when killing a SMX_process */
  void_pfn_smxprocess_t cleanup_process_function;

  xbt_os_mutex_t mutex;
} s_smx_global_t, *smx_global_t;

XBT_PUBLIC_DATA(smx_global_t) simix_global;
extern XBT_PRIVATE unsigned long simix_process_maxpid;

XBT_PUBLIC(void) SIMIX_clean(void);

/******************************** Exceptions *********************************/
/** @brief Ask to the provided simix process to raise the provided exception */
#define SMX_EXCEPTION(issuer, cat, val, msg) \
  if (1) { \
  smx_process_t _smx_throw_issuer = (issuer); /* evaluate only once */ \
  xbt_ex e(msg); \
  e.category = cat; \
  e.value = val; \
  e.procname = xbt_procname(); \
  e.pid = xbt_getpid(); \
  e.file = __FILE__; \
  e.line = __LINE__; \
  e.func = __func__; \
  _smx_throw_issuer->exception = std::make_exception_ptr(e); \
  } else ((void)0)

/* ******************************** File ************************************ */
typedef struct s_smx_file {
  surf_file_t surf_file;
  void* data;                   /**< @brief user data */
} s_smx_file_t;

XBT_PRIVATE void SIMIX_context_mod_init(void);
XBT_PRIVATE void SIMIX_context_mod_exit(void);

XBT_PRIVATE smx_context_t SIMIX_context_new(
  std::function<void()> code,
  void_pfn_smxprocess_t cleanup_func,
  smx_process_t simix_process);

#ifndef WIN32
XBT_PUBLIC_DATA(char sigsegv_stack[SIGSTKSZ]);
#endif

/* We are using the bottom of the stack to save some information, like the
 * valgrind_stack_id. Define smx_context_usable_stack_size to give the remaining
 * size for the stack. */
#if HAVE_VALGRIND_H
# define smx_context_usable_stack_size                                  \
  (smx_context_stack_size - sizeof(unsigned int)) /* for valgrind_stack_id */
#else
# define smx_context_usable_stack_size smx_context_stack_size
#endif

/** @brief Executes all the processes to run (in parallel if possible). */
static inline void SIMIX_context_runall(void)
{
  if (!xbt_dynar_is_empty(simix_global->process_to_run))
    simix_global->context_factory->run_all();
}

/** @brief returns the current running context */
static inline smx_context_t SIMIX_context_self(void)
{
  if (simix_global && simix_global->context_factory)
    return simix_global->context_factory->self();
  else
    return nullptr;
}

XBT_PRIVATE void *SIMIX_context_stack_new(void);
XBT_PRIVATE void SIMIX_context_stack_delete(void *stack);

XBT_PRIVATE void SIMIX_context_set_current(smx_context_t context);
XBT_PRIVATE smx_context_t SIMIX_context_get_current(void);

XBT_PUBLIC(int) SIMIX_process_get_maxpid(void);

XBT_PRIVATE void SIMIX_post_create_environment(void);

// FIXME, Dirty hack for SMPI+MSG
XBT_PRIVATE void SIMIX_process_set_cleanup_function(smx_process_t process, void_pfn_smxprocess_t cleanup);

SG_END_DECL()

#endif
