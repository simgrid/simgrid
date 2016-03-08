/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>

#include <memory>
#include <functional>
#include <utility>

#include <simgrid/simix.hpp>

#include "mc/mc.h"

#include <src/simix/smx_private.h>
#include <src/simix/smx_private.hpp>

void SIMIX_process_set_cleanup_function(
  smx_process_t process, void_pfn_smxprocess_t cleanup)
{
  process->context->set_cleanup(cleanup);
}

/**
 * \brief creates a new context for a user level process
 * \param code a main function
 * \param argc the number of arguments of the main function
 * \param argv the vector of arguments of the main function
 * \param cleanup_func the function to call when the context stops
 * \param cleanup_arg the argument of the cleanup_func function
 */
smx_context_t SIMIX_context_new(
  xbt_main_func_t code, int argc, char **argv,
  void_pfn_smxprocess_t cleanup_func,
  smx_process_t simix_process)
{
  if (!simix_global)
    xbt_die("simix is not initialized, please call MSG_init first");
  return simix_global->context_factory->create_context(
    simgrid::simix::wrap_main(code, argc, argv), cleanup_func, simix_process);
}

namespace simgrid {
namespace simix {

ContextFactoryInitializer factory_initializer = nullptr;

ContextFactory::~ContextFactory() {}

Context* ContextFactory::self()
{
  return SIMIX_context_get_current();
}

void ContextFactory::declare_context(void* context, std::size_t size)
{
#if HAVE_MC
  /* Store the address of the stack in heap to compare it apart of heap comparison */
  if(MC_is_active())
    MC_ignore_heap(context, size);
#endif
}

Context* ContextFactory::attach(void_pfn_smxprocess_t cleanup_func, smx_process_t process)
{
  xbt_die("Cannot attach with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context* ContextFactory::create_maestro(std::function<void()> code, smx_process_t process)
{
  xbt_die("Cannot create_maestro with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context::Context(std::function<void()> code,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process)
  : code_(std::move(code)), process_(process), iwannadie(false)
{
  /* If the user provided a function for the process then use it.
     Otherwise, it is the context for maestro and we should set it as the
     current context */
  if (has_code())
    this->cleanup_func_ = cleanup_func;
  else
    SIMIX_context_set_current(this);
}

Context::~Context()
{
}

void Context::stop()
{
  if (this->cleanup_func_)
    this->cleanup_func_(this->process_);
  this->process_->suspended = 0;

  this->iwannadie = false;
  simcall_process_cleanup(this->process_);
  this->iwannadie = true;
}

AttachContext::~AttachContext()
{
}

}
}
