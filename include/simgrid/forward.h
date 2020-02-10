/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_TYPES_H
#define SIMGRID_TYPES_H

#include <xbt/base.h>

#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>
#include <vector>

namespace simgrid {

namespace s4u {
class Activity;
/** Smart pointer to a simgrid::s4u::Actor */
using ActivityPtr = boost::intrusive_ptr<Activity>;
XBT_PUBLIC void intrusive_ptr_release(const Activity* actor);
XBT_PUBLIC void intrusive_ptr_add_ref(const Activity* actor);

class Actor;
/** Smart pointer to a simgrid::s4u::Actor */
using ActorPtr = boost::intrusive_ptr<Actor>;
XBT_PUBLIC void intrusive_ptr_release(const Actor* actor);
XBT_PUBLIC void intrusive_ptr_add_ref(const Actor* actor);

class Barrier;
/** Smart pointer to a simgrid::s4u::Barrier */
using BarrierPtr = boost::intrusive_ptr<Barrier>;
XBT_PUBLIC void intrusive_ptr_release(Barrier* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Barrier* m);

class Comm;
/** Smart pointer to a simgrid::s4u::Comm */
using CommPtr = boost::intrusive_ptr<Comm>;
XBT_PUBLIC void intrusive_ptr_release(Comm* c);
XBT_PUBLIC void intrusive_ptr_add_ref(Comm* c);

class ConditionVariable;
/** @beginrst
 * Smart pointer to a :cpp:type:`simgrid::s4u::ConditionVariable`
 * @endrst
 */
using ConditionVariablePtr = boost::intrusive_ptr<ConditionVariable>;
XBT_PUBLIC void intrusive_ptr_release(const ConditionVariable* c);
XBT_PUBLIC void intrusive_ptr_add_ref(const ConditionVariable* c);

class Engine;

class Exec;
/** Smart pointer to a simgrid::s4u::Exec */
using ExecPtr = boost::intrusive_ptr<Exec>;
XBT_PUBLIC void intrusive_ptr_release(Exec* e);
XBT_PUBLIC void intrusive_ptr_add_ref(Exec* e);
class ExecSeq; // FIXME: hide this class in implementation
class ExecPar; // FIXME: hide this class in implementation

class Host;

class Io;
/** Smart pointer to a simgrid::s4u::Io */
using IoPtr = boost::intrusive_ptr<Io>;
XBT_PUBLIC void intrusive_ptr_release(Io* i);
XBT_PUBLIC void intrusive_ptr_add_ref(Io* i);

class Link;

class Mailbox;

class Mutex;
XBT_PUBLIC void intrusive_ptr_release(const Mutex* m);
XBT_PUBLIC void intrusive_ptr_add_ref(const Mutex* m);
/**
 * @beginrst
 * Smart pointer to a :cpp:type:`simgrid::s4u::Mutex`
 * @endrst
 */
using MutexPtr = boost::intrusive_ptr<Mutex>;

class NetZone;
class VirtualMachine;
class File;

class Semaphore;
/** Smart pointer to a simgrid::s4u::Semaphore */
using SemaphorePtr = boost::intrusive_ptr<Semaphore>;
XBT_PUBLIC void intrusive_ptr_release(Semaphore* m);
XBT_PUBLIC void intrusive_ptr_add_ref(Semaphore* m);

class Disk;
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

// What's executed as an actor code:
typedef std::function<void()> ActorCode;
// Create an ActorCode from the parameters parsed in the XML file (or elsewhere)
typedef std::function<ActorCode(std::vector<std::string> args)> ActorCodeFactory;
} // namespace actor

namespace activity {
  class ActivityImpl;
  enum class State;
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
  class SemaphoreImpl;
  typedef boost::intrusive_ptr<SemaphoreImpl> SemaphoreImplPtr;
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
class Cpu;
class Model;
class Resource;
class CpuModel;
class NetworkModel;
class LinkImpl;
class NetworkAction;
class DiskImpl;
class DiskModel;
class StorageImpl;
class StorageType;
class StorageModel;
}
namespace routing {
class ClusterCreationArgs;
class LinkCreationArgs;
class NetPoint;
class NetZoneImpl;
class RouteCreationArgs;
}
namespace profile {
class Event;
class FutureEvtSet;
class Profile;
} // namespace profile
} // namespace kernel
namespace simix {
  class Host;
  class Timer;
}
namespace surf {
  class HostImpl;
  class HostModel;
}
namespace mc {
class CommunicationDeterminismChecker;
class SimcallInspector;
}
namespace vm {
class VMModel;
class VirtualMachineImpl;
} // namespace vm
} // namespace simgrid

typedef simgrid::s4u::Actor s4u_Actor;
typedef simgrid::s4u::Barrier s4u_Barrier;
typedef simgrid::s4u::Comm s4u_Comm;
typedef simgrid::s4u::Host s4u_Host;
typedef simgrid::s4u::Link s4u_Link;
typedef simgrid::s4u::File s4u_File;
typedef simgrid::s4u::ConditionVariable s4u_ConditionVariable;
typedef simgrid::s4u::Mailbox s4u_Mailbox;
typedef simgrid::s4u::Mutex s4u_Mutex;
typedef simgrid::s4u::Semaphore s4u_Semaphore;
typedef simgrid::s4u::Disk s4u_Disk;
typedef simgrid::s4u::Storage s4u_Storage;
typedef simgrid::s4u::NetZone s4u_NetZone;
typedef simgrid::s4u::VirtualMachine s4u_VM;

typedef simgrid::simix::Timer* smx_timer_t;
typedef simgrid::kernel::actor::ActorImpl* smx_actor_t;
typedef simgrid::kernel::activity::ActivityImpl* smx_activity_t;
typedef simgrid::kernel::activity::ConditionVariableImpl* smx_cond_t;
typedef simgrid::kernel::activity::MailboxImpl* smx_mailbox_t;
typedef simgrid::kernel::activity::MutexImpl* smx_mutex_t;
typedef simgrid::kernel::activity::SemaphoreImpl* smx_sem_t;
XBT_ATTRIB_DEPRECATED_v330("Please use kernel::activity::State") typedef simgrid::kernel::activity::State e_smx_state_t;
#else

typedef struct s4u_Actor s4u_Actor;
typedef struct s4u_Barrier s4u_Barrier;
typedef struct s4u_Comm s4u_Comm;
typedef struct s4u_Host s4u_Host;
typedef struct s4u_Link s4u_Link;
typedef struct s4u_File s4u_File;
typedef struct s4u_ConditionVariable s4u_ConditionVariable;
typedef struct s4u_Mailbox s4u_Mailbox;
typedef struct s4u_Mutex s4u_Mutex;
typedef struct s4u_Semaphore s4u_Semaphore;
typedef struct s4u_Disk s4u_Disk;
typedef struct s4u_Storage s4u_Storage;
typedef struct s4u_NetZone s4u_NetZone;
typedef struct s4u_VM s4u_VM;
XBT_ATTRIB_DEPRECATED_v330("Please stop using this type alias") typedef enum kernel_activity_state e_smx_state_t;

typedef struct s_smx_timer* smx_timer_t;
typedef struct s_smx_actor* smx_actor_t;
typedef struct s_smx_activity* smx_activity_t;
typedef struct s_smx_cond_t* smx_cond_t;
typedef struct s_smx_mailbox* smx_mailbox_t;
typedef struct s_smx_mutex* smx_mutex_t;
typedef struct s_smx_sem* smx_sem_t;

#endif

/** Pointer to a SimGrid barrier object */
typedef s4u_Barrier* sg_bar_t;
/** Constant pointer to a SimGrid barrier object */
typedef const s4u_Barrier* const_sg_bar_t;
typedef s4u_Comm* sg_comm_t;
typedef const s4u_Comm* const_sg_comm_t;
typedef s4u_ConditionVariable* sg_cond_t;
typedef const s4u_ConditionVariable* const_sg_cond_t;
typedef s4u_Mailbox* sg_mailbox_t;
typedef const s4u_Mailbox* const_sg_mailbox_t;
typedef s4u_Mutex* sg_mutex_t;
typedef const s4u_Mutex* const_sg_mutex_t;
typedef s4u_Semaphore* sg_sem_t;
typedef const s4u_Semaphore* const_sg_sem_t;
typedef s4u_NetZone* sg_netzone_t;
typedef const s4u_NetZone* const_sg_netzone_t;
typedef s4u_Host* sg_host_t;
typedef const s4u_Host* const_sg_host_t;
typedef s4u_Link* sg_link_t;
typedef const s4u_Link* const_sg_link_t;
typedef s4u_Disk* sg_disk_t;
typedef const s4u_Disk* const_sg_disk_t;
typedef s4u_Storage* sg_storage_t;
typedef const s4u_Storage* const_sg_storage_t;
typedef s4u_File* sg_file_t;
typedef const s4u_File* const_sg_file_t;
typedef s4u_VM* sg_vm_t;
typedef const s4u_VM* const_sg_vm_t;
typedef s4u_Actor* sg_actor_t;
typedef const s4u_Actor* const_sg_actor_t;

typedef struct s_smx_simcall* smx_simcall_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid size
 */
typedef unsigned long long sg_size_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid offset
 */
typedef long long sg_offset_t;

/** Actor's ID, just like the classical processes' have PID in UNIX */
typedef long aid_t;

#endif /* SIMGRID_TYPES_H */
