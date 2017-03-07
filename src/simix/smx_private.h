/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_H
#define SIMIX_PRIVATE_H

#include <signal.h>
#include "src/kernel/context/Context.hpp"

#include <map>

/********************************** Simix Global ******************************/

namespace simgrid {
namespace simix {

class Global {
public:
  smx_context_factory_t context_factory = nullptr;
  xbt_dynar_t process_to_run = nullptr;
  xbt_dynar_t process_that_ran = nullptr;
  std::map<int, smx_actor_t> process_list;
#if HAVE_MC
  /* MCer cannot read the std::map above in the remote process, so we copy the info it needs in a dynar.
   * FIXME: This is supposed to be a temporary hack.
   * A better solution would be to change the split between MCer and MCed, where the responsibility
   *   to compute the list of the enabled transitions goes to the MCed.
   * That way, the MCer would not need to have the list of actors on its side.
   * These info could be published by the MCed to the MCer in a way inspired of vd.so
   */
  xbt_dynar_t actors_vector = xbt_dynar_new(sizeof(smx_actor_t), nullptr);
#endif
  xbt_swag_t process_to_destroy = nullptr;
  smx_actor_t maestro_process = nullptr;

  // Maps function names to actor code:
  std::unordered_map<std::string, simgrid::simix::ActorCodeFactory> registered_functions;

  // This might be used when no corresponding function name is registered:
  simgrid::simix::ActorCodeFactory default_function;

  smx_creation_func_t create_process_function = nullptr;
  void_pfn_smxprocess_t kill_process_function = nullptr;
  /** Callback used when killing a SMX_process */
  void_pfn_smxprocess_t cleanup_process_function = nullptr;
  xbt_os_mutex_t mutex = nullptr;

  std::vector<simgrid::xbt::Task<void()>> tasks;
  std::vector<simgrid::xbt::Task<void()>> tasksTemp;
};

}
}

SG_BEGIN_DECL()

XBT_PUBLIC_DATA(std::unique_ptr<simgrid::simix::Global>) simix_global;

XBT_PUBLIC(void) SIMIX_clean();

/******************************** Exceptions *********************************/
/** @brief Ask to the provided simix process to raise the provided exception */
#define SMX_EXCEPTION(issuer, cat, val, msg) \
  if (1) { \
  smx_actor_t _smx_throw_issuer = (issuer); /* evaluate only once */ \
  xbt_ex e(XBT_THROW_POINT, msg); \
  e.category = cat; \
  e.value = val; \
  _smx_throw_issuer->exception = std::make_exception_ptr(e); \
  } else ((void)0)

/* ******************************** File ************************************ */
typedef struct s_smx_file {
  surf_file_t surf_file;
  void* data;                   /**< @brief user data */
} s_smx_file_t;


SG_END_DECL()

#endif
