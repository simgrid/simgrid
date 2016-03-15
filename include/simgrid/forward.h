/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_TYPES_H
#define SG_PLATF_TYPES_H

#ifdef __cplusplus

namespace simgrid {
  namespace s4u {
    class As;
    class Host;
  }
  namespace surf {
    class Resource;
    class Cpu;
    class NetCard;
    class Link;
  }
  namespace trace_mgr {
    class trace;
    class future_evt_set;
  }
}

typedef simgrid::s4u::Host simgrid_Host;
typedef simgrid::s4u::As simgrid_As;
typedef simgrid::surf::Cpu surf_Cpu;
typedef simgrid::surf::NetCard surf_NetCard;
typedef simgrid::surf::Link Link;
typedef simgrid::surf::Resource surf_Resource;
typedef simgrid::trace_mgr::trace tmgr_Trace;

#else

typedef struct simgrid_Host simgrid_Host;
typedef struct simgrid_As   simgrid_As;
typedef struct surf_Cpu surf_Cpu;
typedef struct surf_NetCard surf_NetCard;
typedef struct surf_Resource surf_Resource;
typedef struct Link Link;
typedef struct Trace tmgr_Trace;
#endif

typedef simgrid_Host* sg_host_t;
typedef simgrid_As *AS_t;

typedef surf_Cpu *surf_cpu_t;
typedef surf_NetCard *sg_netcard_t;
typedef surf_Resource *sg_resource_t;

// Types which are in fact dictelmt:
typedef struct s_xbt_dictelm *sg_storage_t;

typedef tmgr_Trace *tmgr_trace_t; /**< Opaque structure defining an availability trace */

typedef enum {
  SURF_LINK_FULLDUPLEX = 2,
  SURF_LINK_SHARED = 1,
  SURF_LINK_FATPIPE = 0
} e_surf_link_sharing_policy_t;

typedef enum {
  SURF_TRACE_CONNECT_KIND_HOST_AVAIL = 4,
  SURF_TRACE_CONNECT_KIND_SPEED = 3,
  SURF_TRACE_CONNECT_KIND_LINK_AVAIL = 2,
  SURF_TRACE_CONNECT_KIND_BANDWIDTH = 1,
  SURF_TRACE_CONNECT_KIND_LATENCY = 0
} e_surf_trace_connect_kind_t;

typedef enum {
  SURF_PROCESS_ON_FAILURE_DIE = 1,
  SURF_PROCESS_ON_FAILURE_RESTART = 0
} e_surf_process_on_failure_t;


/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid size
 */
typedef unsigned long long sg_size_t;

/** @ingroup m_datatypes_management_details
 * @brief Type for any simgrid offset
 */
typedef long long sg_offset_t;

#endif
