/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

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
/** Smart pointer to a simgrid::s4u::Activity */
using ActivityPtr = boost::intrusive_ptr<Activity>;
XBT_PUBLIC void intrusive_ptr_release(const Activity* act);
XBT_PUBLIC void intrusive_ptr_add_ref(const Activity* act);

class ActivitySet;
/** Smart pointer to a simgrid::s4u::Activity */
using ActivitySetPtr = boost::intrusive_ptr<ActivitySet>;
XBT_PUBLIC void intrusive_ptr_release(const ActivitySet* as);
XBT_PUBLIC void intrusive_ptr_add_ref(const ActivitySet* as);

class Actor;
/** Smart pointer to a simgrid::s4u::Actor */
using ActorPtr = boost::intrusive_ptr<Actor>;
XBT_PUBLIC void intrusive_ptr_release(const Actor* actor);
XBT_PUBLIC void intrusive_ptr_add_ref(const Actor* actor);

class Barrier;
/** Smart pointer to a simgrid::s4u::Barrier */
using BarrierPtr = boost::intrusive_ptr<Barrier>;
XBT_PUBLIC void intrusive_ptr_release(Barrier* b);
XBT_PUBLIC void intrusive_ptr_add_ref(Barrier* b);

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

class Host;

class Io;
/** Smart pointer to a simgrid::s4u::Io */
using IoPtr = boost::intrusive_ptr<Io>;
XBT_PUBLIC void intrusive_ptr_release(Io* i);
XBT_PUBLIC void intrusive_ptr_add_ref(Io* i);

class Link;
class SplitDuplexLink;

class Mailbox;

class Mess;
/** Smart pointer to a simgrid::s4u::Mess */
using MessPtr = boost::intrusive_ptr<Mess>;
XBT_PUBLIC void intrusive_ptr_release(Mess* c);
XBT_PUBLIC void intrusive_ptr_add_ref(Mess* c);

class MessageQueue;

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
XBT_PUBLIC void intrusive_ptr_release(const Semaphore* m);
XBT_PUBLIC void intrusive_ptr_add_ref(const Semaphore* m);

class Disk;

class Task;
/** Smart pointer to a simgrid::s4u::Task */
using TaskPtr = boost::intrusive_ptr<Task>;
XBT_PUBLIC void intrusive_ptr_release(Task* o);
XBT_PUBLIC void intrusive_ptr_add_ref(Task* o);
class ExecTask;
using ExecTaskPtr = boost::intrusive_ptr<ExecTask>;
class CommTask;
using CommTaskPtr = boost::intrusive_ptr<CommTask>;
class IoTask;
using IoTaskPtr = boost::intrusive_ptr<IoTask>;

/**
 * @brief Callback to dynamically change the resource's capacity
 *
 * Allows user to change resource's capacity depending on the number of concurrent activities
 * running on the resource at a given instant
 */
using NonLinearResourceCb = std::function<double(double capacity, int n_activities)>;
} // namespace s4u

namespace config {
template <class T> class Flag;
}

namespace kernel {
class EngineImpl;
namespace actor {
class ActorImpl;
using ActorImplPtr = boost::intrusive_ptr<ActorImpl>;

// What's executed as an actor code:
using ActorCode = std::function<void()>;
// Create an ActorCode from the parameters parsed in the XML file (or elsewhere)
using ActorCodeFactory = std::function<ActorCode(std::vector<std::string> args)>;

class Simcall;
class SimcallObserver;
class MutexObserver;
class ConditionVariableObserver;
class ObjectAccessSimcallObserver;
class ObjectAccessSimcallItem;
} // namespace actor

namespace activity {
  class ActivityImpl;
  enum class State;
  using ActivityImplPtr = boost::intrusive_ptr<ActivityImpl>;
  XBT_PUBLIC void intrusive_ptr_add_ref(ActivityImpl* activity);
  XBT_PUBLIC void intrusive_ptr_release(ActivityImpl* activity);

  class BarrierImpl;
  using BarrierImplPtr = boost::intrusive_ptr<BarrierImpl>;
  XBT_PUBLIC void intrusive_ptr_add_ref(BarrierImpl* cond);
  XBT_PUBLIC void intrusive_ptr_release(BarrierImpl* cond);
  class BarrierAcquisitionImpl;
  using BarrierAcquisitionImplPtr = boost::intrusive_ptr<BarrierAcquisitionImpl>;

  class ConditionVariableImpl;
  using ConditionVariableImplPtr = boost::intrusive_ptr<ConditionVariableImpl>;
  XBT_PUBLIC void intrusive_ptr_add_ref(ConditionVariableImpl* cond);
  XBT_PUBLIC void intrusive_ptr_release(ConditionVariableImpl* cond);
  class ConditionVariableAcquisitionImpl;
  using ConditionVariableAcquisitionImplPtr = boost::intrusive_ptr<ConditionVariableAcquisitionImpl>;

  class CommImpl;
  using CommImplPtr = boost::intrusive_ptr<CommImpl>;
  class ExecImpl;
  using ExecImplPtr = boost::intrusive_ptr<ExecImpl>;
  class IoImpl;
  using IoImplPtr = boost::intrusive_ptr<IoImpl>;
  class MessImpl;
  using MessImplPtr = boost::intrusive_ptr<MessImpl>;
  class MutexImpl;
  using MutexImplPtr = boost::intrusive_ptr<MutexImpl>;
  class MutexAcquisitionImpl;
  using MutexAcquisitionImplPtr = boost::intrusive_ptr<MutexAcquisitionImpl>;
  XBT_PUBLIC void intrusive_ptr_add_ref(MutexImpl* mutex);
  XBT_PUBLIC void intrusive_ptr_release(MutexImpl* mutex);
  class SynchroImpl;
  using SynchroImplPtr = boost::intrusive_ptr<SynchroImpl>;
  class SemaphoreImpl;
  using SemaphoreImplPtr = boost::intrusive_ptr<SemaphoreImpl>;
  class SemAcquisitionImpl;
  using SemAcquisitionImplPtr = boost::intrusive_ptr<SemAcquisitionImpl>;
  XBT_PUBLIC void intrusive_ptr_add_ref(SemaphoreImpl* sem);
  XBT_PUBLIC void intrusive_ptr_release(SemaphoreImpl* sem);
  class SleepImpl;
  using SleepImplPtr = boost::intrusive_ptr<SleepImpl>;

  class MailboxImpl;
  class MessageQueueImpl;
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
class CpuAction;
class CpuImpl;
class Model;
class Resource;
class CpuModel;
class HostImpl;
class HostModel;
class NetworkModel;
class LinkImpl;
class StandardLinkImpl;
class SplitDuplexLinkImpl;
class NetworkAction;
class DiskImpl;
using DiskImplPtr = boost::intrusive_ptr<DiskImpl>;
class DiskModel;
class VirtualMachineImpl;
class VMModel;
}
namespace timer {
class Timer;
}
namespace routing {
class NetPoint;
class NetZoneImpl;
}
namespace profile {
class Event;
class FutureEvtSet;
class Profile;
} // namespace profile
} // namespace kernel
namespace mc {
class State;
class RemoteApp;
}
} // namespace simgrid

using s4u_Actor             = simgrid::s4u::Actor;
using s4u_Activity          = simgrid::s4u::Activity;
using s4u_ActivitySet       = simgrid::s4u::ActivitySet;
using s4u_Barrier           = simgrid::s4u::Barrier;
using s4u_Comm              = simgrid::s4u::Comm;
using s4u_Exec              = simgrid::s4u::Exec;
using s4u_Host              = simgrid::s4u::Host;
using s4u_Link              = simgrid::s4u::Link;
using s4u_File              = simgrid::s4u::File;
using s4u_ConditionVariable = simgrid::s4u::ConditionVariable;
using s4u_Mailbox           = simgrid::s4u::Mailbox;
using s4u_MessageQueue      = simgrid::s4u::MessageQueue;
using s4u_Mutex             = simgrid::s4u::Mutex;
using s4u_Semaphore         = simgrid::s4u::Semaphore;
using s4u_Disk              = simgrid::s4u::Disk;
using s4u_NetZone           = simgrid::s4u::NetZone;
using s4u_VM                = simgrid::s4u::VirtualMachine;

using smx_activity_t = simgrid::kernel::activity::ActivityImpl*;
#else

typedef struct s4u_Actor s4u_Actor;
typedef struct s4u_Activity s4u_Activity;
typedef struct s4u_ActivitySet s4u_ActivitySet;
typedef struct s4u_Barrier s4u_Barrier;
typedef struct s4u_Comm s4u_Comm;
typedef struct s4u_Exec s4u_Exec;
typedef struct s4u_Host s4u_Host;
typedef struct s4u_Link s4u_Link;
typedef struct s4u_File s4u_File;
typedef struct s4u_ConditionVariable s4u_ConditionVariable;
typedef struct s4u_Mailbox s4u_Mailbox;
typedef struct s4u_MessageQueue s4u_MessageQueue;
typedef struct s4u_Mutex s4u_Mutex;
typedef struct s4u_Semaphore s4u_Semaphore;
typedef struct s4u_Disk s4u_Disk;
typedef struct s4u_NetZone s4u_NetZone;
typedef struct s4u_VM s4u_VM;

typedef struct s_smx_activity* smx_activity_t;

#endif

/** Pointer to a SimGrid barrier object */
typedef s4u_Barrier* sg_bar_t;
typedef s4u_Comm* sg_comm_t;
typedef s4u_Exec* sg_exec_t;
typedef const s4u_Exec* const_sg_exec_t;
typedef s4u_ConditionVariable* sg_cond_t;
typedef const s4u_ConditionVariable* const_sg_cond_t;
typedef s4u_Mailbox* sg_mailbox_t;
typedef const s4u_Mailbox* const_sg_mailbox_t;
typedef s4u_MessageQueue* sg_messagequeue_t;
typedef const s4u_MessageQueue* const_sg_messagequeue_t;
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
/** Pointer to a SimGrid file object @ingroup plugin_filesystem */
typedef s4u_File* sg_file_t;
/** Constant pointer to a SimGrid file object @ingroup plugin_filesystem */
typedef const s4u_File* const_sg_file_t;
typedef s4u_VM* sg_vm_t;
typedef const s4u_VM* const_sg_vm_t;
/** Pointer to an actor object */
typedef s4u_Actor* sg_actor_t;
/** Pointer to a constant actor object */
typedef const s4u_Actor* const_sg_actor_t;

/** Pointer to an activity object */
typedef s4u_Activity* sg_activity_t;
/** Pointer to a constant activity object */
typedef const s4u_Activity* const_sg_activity_t;

/** Pointer to an activity set object */
typedef s4u_ActivitySet* sg_activity_set_t;
/** Pointer to a constant activity set object */
typedef const s4u_ActivitySet* const_sg_activity_set_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any SimGrid size
 */
typedef unsigned long long sg_size_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any SimGrid offset
 */
typedef long long sg_offset_t;

/** Actor's ID, just like the classical processes' have PID in UNIX */
typedef long aid_t;

typedef enum {
  SG_OK /** Code returned when no problem occured */,
  SG_ERROR_CANCELED /** Code returned when something got canceled before completion */,
  SG_ERROR_TIMEOUT /** Code returned when timeout elapsed */,
  SG_ERROR_HOST /** Code returned when a host fails */,
  SG_ERROR_NETWORK /** Code returned when a communication fails because of the network or because of the remote host */,
  SG_ERROR_STORAGE /** Code returned when a storage fails */,
  SG_ERROR_VM /** Code returned when a VM fails */
} sg_error_t;

XBT_PUBLIC int SMPI_is_inited();

#endif /* SIMGRID_TYPES_H */
