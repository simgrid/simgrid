/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/context_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

#include "ContextSwapped.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace kernel {
namespace context {

unsigned long SwappedContext::process_index_;

SwappedContext* SwappedContext::maestro_context_ = nullptr;

void SwappedContext::suspend()
{
  /* determine the next context */
  SwappedContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;

  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<SwappedContext*>(simix_global->process_to_run[i]->context_);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<SwappedContext*>(get_maestro());
  }
  Context::set_current(next_context);
  this->swap_into(next_context);
}

void SwappedContext::resume()
{
  SwappedContext* old = static_cast<SwappedContext*>(self());
  Context::set_current(this);
  old->swap_into(this);
}

void SwappedContext::run_all()
{
  if (simix_global->process_to_run.empty())
    return;
  smx_actor_t first_process = simix_global->process_to_run.front();
  process_index_            = 1;
  /* execute the first process */
  static_cast<SwappedContext*>(first_process->context_)->resume();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
