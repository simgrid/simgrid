/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/simix.hpp>

#include "mc/mc.h"

#include <src/simix/smx_private.h>

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
#ifdef HAVE_MC
  /* Store the address of the stack in heap to compare it apart of heap comparison */
  if(MC_is_active())
    MC_ignore_heap(context, size);
#endif
}

Context::Context(xbt_main_func_t code,
        int argc, char **argv,
        void_pfn_smxprocess_t cleanup_func,
        smx_process_t process)
  : process_(process), iwannadie(false)
{
  /* If the user provided a function for the process then use it.
     Otherwise, it is the context for maestro and we should set it as the
     current context */
  if (code) {
    this->cleanup_func_ = cleanup_func;
    this->argc_ = argc;
    this->argv_ = argv;
    this->code_ = code;
  } else {
    SIMIX_context_set_current(this);
  }
}

Context::~Context()
{
  if (this->argv_) {
    for (int i = 0; i < this->argc_; i++)
      free(this->argv_[i]);
    free(this->argv_);
  }
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

}
}