/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_TYPES_H
#define SIMGRID_TYPES_H

#include <xbt/base.h>

#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>

namespace simgrid {

namespace s4u {
class Activity;

class Actor;
/** Smart pointer to a simgrid::s4u::Actor */
typedef boost::intrusive_ptr<Actor> ActorPtr;
XBT_PUBLIC void intrusive_ptr_release(Actor* actor);
XBT_PUBLIC void intrusive_ptr_add_ref(Actor* actor);

class Barrier;
/** Smart pointer to a simgrid::s4u::Barrier */
typedef boost::intrusive_ptr<Barrier> BarrierPtr;
XBT_PUBLIC void intrusive_ptr_release(Barrier* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Barrier* m);

class Comm;
/** Smart pointer to a simgrid::s4u::Comm */
typedef boost::intrusive_ptr<Comm> CommPtr;
XBT_PUBLIC void intrusive_ptr_release(Comm* c);
XBT_PUBLIC void intrusive_ptr_add_ref(Comm* c);

class ConditionVariable;
/** Smart pointer to a simgrid::s4u::ConditionVariable */
typedef boost::intrusive_ptr<ConditionVariable> ConditionVariablePtr;
XBT_PUBLIC void intrusive_ptr_release(ConditionVariable* c);
XBT_PUBLIC void intrusive_ptr_add_ref(ConditionVariable* c);

class Engine;

class Exec;
/** Smart pointer to a simgrid::s4u::Exec */
typedef boost::intrusive_ptr<Exec> ExecPtr;
XBT_PUBLIC void intrusive_ptr_release(Exec* e);
XBT_PUBLIC void intrusive_ptr_add_ref(Exec* e);

class Host;

class Io;
/** Smart pointer to a simgrid::s4u::Io */
typedef boost::intrusive_ptr<Io> IoPtr;
XBT_PUBLIC void intrusive_ptr_release(Io* i);
XBT_PUBLIC void intrusive_ptr_add_ref(Io* i);

class Link;

class Mailbox;
/** Smart pointer to a simgrid::s4u::Mailbox */
typedef boost::intrusive_ptr<Mailbox> MailboxPtr;
XBT_PUBLIC void intrusive_ptr_release(Mailbox* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Mailbox* m);

class Mutex;
XBT_PUBLIC void intrusive_ptr_release(Mutex* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Mutex* m);
/** Smart pointer to a simgrid::s4u::Mutex */
typedef boost::intrusive_ptr<Mutex> MutexPtr;

class NetZone;
class VirtualMachine;
class File;

class Semaphore;
/** Smart pointer to a simgrid::s4u::Semaphore */
typedef boost::intrusive_ptr<Semaphore> SemaphorePtr;
XBT_PUBLIC void intrusive_ptr_release(Semaphore* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Semaphore* m);

class Storage;
} // namespace s4u

namespace config {
template <class T> class Flag;
}

namespace kernel {
class EngineImpl;
namespace actor {
class ActorImpl;
typedef boost::intrusive_ptr<ActorImpl> ActorImplPtr;
} // namespace actor

namespace activity {
  class ActivityImpl;
  typedef boost::intrusive_ptr<ActivityImpl> ActivityImplPtr;
  XBT_PUBLIC void intrusive_ptr_add_ref(ActivityImpl* activity);
  XBT_PUBLIC void intrusive_ptr_release(ActivityImpl* activity);

  class ConditionVariableImpl;

  class CommImpl;
  typedef boost::intrusive_ptr<CommImpl> CommImplPtr;
  class ExecImpl;
  typedef boost::intrusive_ptr<ExecImpl> ExecImplPtr;
  class IoImpl;
  typedef boost::intrusive_ptr<IoImpl> IoImplPtr;
  class MutexImpl;
  typedef boost::intrusive_ptr<MutexImpl> MutexImplPtr;
  class RawImpl;
  typedef boost::intrusive_ptr<RawImpl> RawImplPtr;
  class SleepImpl;
  typedef boost::intrusive_ptr<SleepImpl> SleepImplPtr;

  class MailboxImpl;
}
namespace context {
class Context;
class ContextFactory;
} // namespace context
namespace lmm {
class Element;
class Variable;
class Constraint;
class ConstraintLight;
class System;
}
namespace resource {
class Action;
class Model;
class Resource;
class NetworkModel;
class TraceEvent;
class LinkImpl;
class NetworkAction;
}
namespace routing {
class ClusterCreationArgs;
class LinkCreationArgs;
class NetPoint;
class NetZoneImpl;
class RouteCreationArgs;
}
} // namespace kernel
namespace simix {
  class Host;
}
namespace surf {
  class Cpu;
  class CpuModel;
  class HostImpl;
  class HostModel;
  class StorageImpl;
  class StorageType;
  class StorageModel;
}
namespace mc {
class CommunicationDeterminismChecker;
}
namespace trace_mgr {
  class trace;
  class future_evt_set;
}
namespace vm {
class VMModel;
class VirtualMachineImpl;
} // namespace vm
} // namespace simgrid

typedef simgrid::s4u::Actor s4u_Actor;
typedef simgrid::s4u::Barrier s4u_Barrier;
typedef simgrid::s4u::Host s4u_Host;
typedef simgrid::s4u::Link s4u_Link;
typedef simgrid::s4u::File s4u_File;
typedef simgrid::s4u::Storage s4u_Storage;
typedef simgrid::s4u::NetZone s4u_NetZone;
typedef simgrid::s4u::VirtualMachine s4u_VM;
typedef boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> smx_activity_t;
typedef simgrid::trace_mgr::trace* tmgr_trace_t;

typedef simgrid::kernel::context::Context* smx_context_t;
typedef simgrid::kernel::actor::ActorImpl* smx_actor_t;
typedef simgrid::kernel::activity::ConditionVariableImpl* smx_cond_t;
typedef simgrid::kernel::activity::MutexImpl* smx_mutex_t;
typedef simgrid::kernel::activity::MailboxImpl* smx_mailbox_t;

#else

typedef struct s4u_Actor s4u_Actor;
typedef struct s4u_Barrier s4u_Barrier;
typedef struct s4u_Host s4u_Host;
typedef struct s4u_Link s4u_Link;
typedef struct s4u_File s4u_File;
typedef struct s4u_Storage s4u_Storage;
typedef struct s4u_NetZone s4u_NetZone;
typedef struct s4u_VM s4u_VM;
typedef struct kernel_Activity* smx_activity_t;

typedef struct s_smx_context* smx_context_t;
typedef struct s_smx_actor* smx_actor_t;
typedef struct s_smx_cond_t* smx_cond_t;
typedef struct s_smx_mutex* smx_mutex_t;
typedef struct s_smx_mailbox* smx_mailbox_t;

#endif

typedef s4u_Barrier* sg_bar_t;
typedef s4u_NetZone* sg_netzone_t;
typedef s4u_Host* sg_host_t;
typedef s4u_Link* sg_link_t;
typedef s4u_Storage* sg_storage_t;
typedef s4u_File* sg_file_t;
typedef s4u_VM* sg_vm_t;
typedef s4u_Actor* sg_actor_t;

typedef struct s_smx_simcall* smx_simcall_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid size
 */
typedef unsigned long long sg_size_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid offset
 */
typedef long long sg_offset_t;

typedef long aid_t;

#endif /* SIMGRID_TYPES_H */
