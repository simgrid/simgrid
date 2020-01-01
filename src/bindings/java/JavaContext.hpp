/* Context switching within the JVM.                                        */

/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_JAVA_JAVA_CONTEXT_HPP
#define SIMGRID_JAVA_JAVA_CONTEXT_HPP

#include <functional>
#include <jni.h>

#include "simgrid/simix.h"
#include "src/kernel/context/ContextThread.hpp"
#include "src/simix/smx_private.hpp"

#include "jmsg.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class JavaContext;
class JavacontextFactory;

class JavaContext : public simgrid::kernel::context::SerialThreadContext {
public:
  // The java process instance bound with the msg process structure:
  jobject jprocess_ = nullptr;
  // JNI interface pointer associated to this thread:
  JNIEnv* jenv_           = nullptr;

  friend class JavaContextFactory;
  JavaContext(std::function<void()>&& code, smx_actor_t actor);

  void start_hook() override;
  void stop_hook() override;
};

class JavaContextFactory : public simgrid::kernel::context::ContextFactory {
public:
  JavaContextFactory();
  ~JavaContextFactory() override;
  Context* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override;
  void run_all() override;
};

XBT_PRIVATE simgrid::kernel::context::ContextFactory* java_factory();
XBT_PRIVATE void java_main_jprocess(jobject process);

}}} // namespace simgrid::kernel::context

#endif                          /* !_XBT_CONTEXT_JAVA_H */
