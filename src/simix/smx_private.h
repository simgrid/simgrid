/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PRIVATE_H
#define _SIMIX_PRIVATE_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <xbt/functional.hpp>

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
#include "smx_host_private.h"
#include "smx_io_private.h"
#include "smx_network_private.h"
#include "popping_private.h"
#include "smx_synchro_private.h"

#include <signal.h>
#include "src/simix/ActorImpl.hpp"
#include "src/kernel/context/Context.hpp"

/********************************** Simix Global ******************************/

namespace simgrid {
namespace simix {

class Global {
public:
  smx_context_factory_t context_factory = nullptr;
  xbt_dynar_t process_to_run = nullptr;
  xbt_dynar_t process_that_ran = nullptr;
  xbt_swag_t process_list = nullptr;
  xbt_swag_t process_to_destroy = nullptr;
  smx_process_t maestro_process = nullptr;

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

XBT_PUBLIC(void) SIMIX_clean(void);

/******************************** Exceptions *********************************/
/** @brief Ask to the provided simix process to raise the provided exception */
#define SMX_EXCEPTION(issuer, cat, val, msg) \
  if (1) { \
  smx_process_t _smx_throw_issuer = (issuer); /* evaluate only once */ \
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
