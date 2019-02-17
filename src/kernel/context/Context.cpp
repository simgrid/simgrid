/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);


namespace simgrid {
namespace kernel {
namespace context {

ContextFactoryInitializer factory_initializer = nullptr;

ContextFactory::~ContextFactory() = default;

static thread_local smx_context_t smx_current_context = nullptr;
Context* Context::self()
{
  return smx_current_context;
}
void Context::set_current(Context* self)
{
  smx_current_context = self;
}

void Context::declare_context(std::size_t size)
{
#if SIMGRID_HAVE_MC
  /* Store the address of the stack in heap to compare it apart of heap comparison */
  if(MC_is_active())
    MC_ignore_heap(this, size);
#endif
}

Context* ContextFactory::attach(smx_actor_t)
{
  xbt_die("Cannot attach with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context* ContextFactory::create_maestro(std::function<void()>, smx_actor_t)
{
  xbt_die("Cannot create_maestro with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context::Context(std::function<void()> code, smx_actor_t actor) : code_(std::move(code)), actor_(actor)
{
  /* If no function was provided, this is the context for maestro
   * and we should set it as the current context */
  if (not has_code())
    set_current(this);
}

Context::~Context()
{
  if (self() == this)
    set_current(nullptr);
}

void Context::stop()
{
  actor_->finished_ = true;

  if (actor_->has_to_auto_restart() && not actor_->get_host()->is_on()) {
    XBT_DEBUG("Insert host %s to watched_hosts because it's off and %s needs to restart",
              actor_->get_host()->get_cname(), actor_->get_cname());
    watched_hosts.insert(actor_->get_host()->get_name());
  }

  // Execute the termination callbacks
  smx_process_exit_status_t exit_status = (actor_->context_->iwannadie) ? SMX_EXIT_FAILURE : SMX_EXIT_SUCCESS;
  while (not actor_->on_exit.empty()) {
    s_smx_process_exit_fun_t exit_fun = actor_->on_exit.back();
    actor_->on_exit.pop_back();
    (exit_fun.fun)(exit_status, exit_fun.arg);
  }

  /* cancel non-blocking activities */
  for (auto activity : actor_->comms)
    boost::static_pointer_cast<activity::CommImpl>(activity)->cancel();
  actor_->comms.clear();

  XBT_DEBUG("%s@%s(%ld) should not run anymore", actor_->get_cname(), actor_->iface()->get_host()->get_cname(),
            actor_->get_pid());

  this->actor_->cleanup();

  this->iwannadie = false; // don't let the simcall's yield() do a Context::stop(), because that's me
  simgrid::simix::simcall([this] {
    simgrid::s4u::Actor::on_destruction(actor_->iface());

    /* Unregister from the kill timer if any */
    if (actor_->kill_timer != nullptr) {
      SIMIX_timer_remove(actor_->kill_timer);
      actor_->kill_timer = nullptr;
    }

    actor_->cleanup();
  });
  this->iwannadie = true;
}

AttachContext::~AttachContext() = default;

StopRequest::~StopRequest() = default;

void StopRequest::do_throw()
{
  throw StopRequest();
}

bool StopRequest::try_n_catch(std::function<void(void)> try_block)
{
  bool res;
  try {
    try_block();
    res = true;
  } catch (StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
    res = false;
  }
  return res;
}
}}}

/** @brief Executes all the processes to run (in parallel if possible). */
void SIMIX_context_runall()
{
  simix_global->context_factory->run_all();
}
