/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_TYPES_H
#define SIMGRID_TYPES_H

#ifdef __cplusplus

#include "s4u/forward.hpp"
#include "xbt/base.h"
#include <boost/intrusive_ptr.hpp>

namespace simgrid {
namespace kernel {
namespace context {
class Context;
class ContextFactory;
}
namespace activity {
  class ActivityImpl;
  using ActivityImplPtr = boost::intrusive_ptr<ActivityImpl>;
  XBT_PUBLIC(void) intrusive_ptr_add_ref(ActivityImpl* activity);
  XBT_PUBLIC(void) intrusive_ptr_release(ActivityImpl* activity);

  class CommImpl;
  using CommImplPtr = boost::intrusive_ptr<CommImpl>;
  class ExecImpl;
  using ExecImplPtr = boost::intrusive_ptr<ExecImpl>;
  class IoImpl;
  using IoImplPtr = boost::intrusive_ptr<IoImpl>;
  class RawImpl;
  using RawImplPtr = boost::intrusive_ptr<RawImpl>;
  class SleepImpl;
  using SleepImplPtr = boost::intrusive_ptr<SleepImpl>;

  class MailboxImpl;
}
namespace lmm {
class Element;
class Variable;
class Constraint;
class ConstraintLight;
class System;
}
namespace routing {
  class NetPoint;
  class NetZoneImpl;
}
}
namespace simix {
  class ActorImpl;
  using ActorImplPtr = boost::intrusive_ptr<ActorImpl>;
  class Host;

  class MutexImpl;
}
namespace surf {
  class Resource;
  class Cpu;
  class LinkImpl;
  class HostImpl;
  class StorageImpl;
  class StorageType;
}
namespace trace_mgr {
  class trace;
  class future_evt_set;
}
}

typedef simgrid::s4u::Actor s4u_Actor;
typedef simgrid::s4u::Host s4u_Host;
typedef simgrid::s4u::Link s4u_Link;
typedef simgrid::s4u::File s4u_File;
typedef simgrid::s4u::Storage s4u_Storage;
typedef simgrid::s4u::NetZone s4u_NetZone;
typedef simgrid::s4u::VirtualMachine s4u_VM;
typedef boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> smx_activity_t;
typedef simgrid::kernel::routing::NetPoint routing_NetPoint;
typedef simgrid::surf::Resource surf_Resource;
typedef simgrid::trace_mgr::trace tmgr_Trace;

typedef simgrid::kernel::context::Context* smx_context_t;
typedef simgrid::simix::ActorImpl* smx_actor_t;
typedef simgrid::simix::MutexImpl* smx_mutex_t;
typedef simgrid::kernel::activity::MailboxImpl* smx_mailbox_t;
typedef simgrid::surf::StorageImpl* surf_storage_t;

typedef simgrid::kernel::lmm::Element* lmm_element_t;
typedef const simgrid::kernel::lmm::Element* const_lmm_element_t;
typedef simgrid::kernel::lmm::Variable* lmm_variable_t;
typedef simgrid::kernel::lmm::Constraint* lmm_constraint_t;
typedef simgrid::kernel::lmm::ConstraintLight* lmm_constraint_light_t;
typedef simgrid::kernel::lmm::System* lmm_system_t;

#else

typedef struct s4u_Actor s4u_Actor;
typedef struct s4u_Host s4u_Host;
typedef struct s4u_Link s4u_Link;
typedef struct s4u_File s4u_File;
typedef struct s4u_Storage s4u_Storage;
typedef struct s4u_NetZone s4u_NetZone;
typedef struct s4u_VM s4u_VM;
typedef struct kernel_Activity* smx_activity_t;
typedef struct routing_NetPoint routing_NetPoint;
typedef struct surf_Resource surf_Resource;
typedef struct Trace tmgr_Trace;

typedef struct s_smx_context* smx_context_t;
typedef struct s_smx_actor* smx_actor_t;
typedef struct s_smx_mutex* smx_mutex_t;
typedef struct s_smx_mailbox* smx_mailbox_t;
typedef struct s_surf_storage* surf_storage_t;

#endif

typedef s4u_NetZone* sg_netzone_t;
typedef s4u_Host* sg_host_t;
typedef s4u_Link* sg_link_t;
typedef s4u_Storage* sg_storage_t;
typedef s4u_File* sg_file_t;
typedef s4u_VM* sg_vm_t;

typedef routing_NetPoint* sg_netpoint_t;
typedef surf_Resource *sg_resource_t;

typedef tmgr_Trace *tmgr_trace_t; /**< Opaque structure defining an availability trace */

typedef struct s_smx_simcall s_smx_simcall_t;
typedef struct s_smx_simcall* smx_simcall_t;

typedef enum { // FIXME: move this to s4u::Link; make it an enum class
  SURF_LINK_FULLDUPLEX = 2,
  SURF_LINK_SHARED     = 1,
  SURF_LINK_FATPIPE    = 0
} e_surf_link_sharing_policy_t;

typedef enum { // FIXME: move this away; make it an enum class
  SURF_TRACE_CONNECT_KIND_HOST_AVAIL = 4,
  SURF_TRACE_CONNECT_KIND_SPEED      = 3,
  SURF_TRACE_CONNECT_KIND_LINK_AVAIL = 2,
  SURF_TRACE_CONNECT_KIND_BANDWIDTH  = 1,
  SURF_TRACE_CONNECT_KIND_LATENCY    = 0
} e_surf_trace_connect_kind_t;

typedef enum { // FIXME: move this to s4u::Actor; make it an enum class
  SURF_ACTOR_ON_FAILURE_DIE     = 1,
  SURF_ACTOR_ON_FAILURE_RESTART = 0
} e_surf_process_on_failure_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid size
 */
typedef unsigned long long sg_size_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid offset
 */
typedef long long sg_offset_t;

typedef unsigned long aid_t;

#endif /* SIMGRID_TYPES_H */
