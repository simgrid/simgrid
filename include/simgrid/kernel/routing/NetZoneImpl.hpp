/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_NETZONEIMPL_HPP
#define SIMGRID_ROUTING_NETZONEIMPL_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Link.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <xbt/PropertyHolder.hpp>
#include <xbt/graph.h>

#include <map>
#include <vector>

namespace simgrid {
namespace kernel {
namespace routing {

class Route {
public:
  Route() = default;
  explicit Route(NetPoint* src, NetPoint* dst, NetPoint* gwSrc, NetPoint* gwDst)
      : src_(src), dst_(dst), gw_src_(gwSrc), gw_dst_(gwDst)
  {
  }
  NetPoint* src_    = nullptr;
  NetPoint* dst_    = nullptr;
  NetPoint* gw_src_ = nullptr;
  NetPoint* gw_dst_ = nullptr;
  std::vector<resource::LinkImpl*> link_list_;
};

class BypassRoute {
public:
  explicit BypassRoute(NetPoint* gwSrc, NetPoint* gwDst) : gw_src(gwSrc), gw_dst(gwDst) {}
  NetPoint* gw_src;
  NetPoint* gw_dst;
  std::vector<resource::LinkImpl*> links;
};

/** @ingroup ROUTING_API
 *  @brief Private implementation of the Networking Zones
 *
 * A netzone is a network container, in charge of routing information between elements (hosts and sub-netzones)
 * and to the nearby netzones. In SimGrid, there is a hierarchy of netzones, ie a tree with a unique root
 * NetZone, that you can retrieve with simgrid::s4u::Engine::netRoot().
 *
 * The purpose of the kernel::routing module is to retrieve the routing path between two points in a time- and
 * space-efficient manner. This is done by NetZoneImpl::getGlobalRoute(), called when creating a communication to
 * retrieve both the list of links that the create communication will use, and the summed latency that these
 * links represent.
 *
 * The network model could recompute the latency by itself from the list, but it would require an additional
 * traversal of the link set. This operation being on the critical path of SimGrid, the routing computes the
 * latency on the behalf of the network while constructing the link set.
 *
 * Finding the path between two nodes is rather complex because we navigate a hierarchy of netzones, each of them
 * being a full network. In addition, the routing can declare shortcuts (called bypasses), either within a NetZone
 * at the route level or directly between NetZones. Also, each NetZone can use a differing routing algorithm, depending
 * on its class. @ref FullZone have a full matrix giving explicitly the path between any pair of their
 * contained nodes, while @ref DijkstraZone or @ref FloydZone rely on a shortest path algorithm. @ref VivaldiZone
 * does not even have any link but only use only coordinate information to compute the latency.
 *
 * So NetZoneImpl::getGlobalRoute builds the path recursively asking its specific information to each traversed NetZone
 * with NetZoneImpl::getLocalRoute, that is redefined in each sub-class.
 * The algorithm for that is explained in http://hal.inria.fr/hal-00650233/ (but for historical reasons, NetZones are
 * called Autonomous Systems in this article).
 *
 */
class XBT_PUBLIC NetZoneImpl : public xbt::PropertyHolder {
  friend EngineImpl; // it destroys netRoot_
  s4u::NetZone piface_;

  // our content, as known to our graph routing algorithm (maps vertex_id -> vertex)
  std::vector<kernel::routing::NetPoint*> vertices_;

  NetZoneImpl* parent_ = nullptr;
  std::vector<NetZoneImpl*> children_; // sub-netzones
  std::string name_;
  bool sealed_ = false; // We cannot add more content when sealed

  std::map<std::pair<NetPoint*, NetPoint*>, BypassRoute*> bypass_routes_; // src x dst -> route
  routing::NetPoint* netpoint_ = nullptr;                                 // Our representative in the father NetZone

protected:
  explicit NetZoneImpl(const std::string& name);
  NetZoneImpl(const NetZoneImpl&) = delete;
  NetZoneImpl& operator=(const NetZoneImpl&) = delete;
  virtual ~NetZoneImpl();

  /**
   * @brief Probe the routing path between two points that are local to the called NetZone.
   *
   * @param src where from
   * @param dst where to
   * @param into Container into which the traversed links and gateway information should be pushed
   * @param latency Accumulator in which the latencies should be added (caller must set it to 0)
   */
  virtual void get_local_route(NetPoint* src, NetPoint* dst, Route* into, double* latency) = 0;
  /** @brief retrieves the list of all routes of size 1 (of type src x dst x Link) */
  /* returns whether we found a bypass path */
  bool get_bypass_route(routing::NetPoint* src, routing::NetPoint* dst,
                        /* OUT */ std::vector<resource::LinkImpl*>& links, double* latency);

public:
  enum class RoutingMode {
    base,     /**< Base case: use simple link lists for routing     */
    recursive /**< Recursive case: also return gateway information  */
  };

  /** @brief Retrieves the network model associated to this NetZone */
  const std::shared_ptr<resource::NetworkModel>& get_network_model() const { return network_model_; }
  /** @brief Retrieves the CPU model for virtual machines associated to this NetZone */
  const std::shared_ptr<resource::CpuModel>& get_cpu_vm_model() const { return cpu_model_vm_; }
  /** @brief Retrieves the CPU model for physical machines associated to this NetZone */
  const std::shared_ptr<resource::CpuModel>& get_cpu_pm_model() const { return cpu_model_pm_; }
  /** @brief Retrieves the disk model associated to this NetZone */
  const std::shared_ptr<resource::DiskModel>& get_disk_model() const { return disk_model_; }
  /** @brief Retrieves the host model associated to this NetZone */
  const std::shared_ptr<surf::HostModel>& get_host_model() const { return host_model_; }

  const s4u::NetZone* get_iface() const { return &piface_; }
  s4u::NetZone* get_iface() { return &piface_; }
  unsigned int get_table_size() const { return vertices_.size(); }
  std::vector<kernel::routing::NetPoint*> get_vertices() const { return vertices_; }
  XBT_ATTRIB_DEPRECATED_v331("Please use get_parent()") NetZoneImpl* get_father() const { return parent_; }
  NetZoneImpl* get_parent() const { return parent_; }
  /** @brief Returns the list of direct children (no grand-children). This returns the internal data, no copy.
   * Don't mess with it.*/
  const std::vector<NetZoneImpl*>& get_children() const { return children_; }
  /** @brief Get current netzone hierarchy */
  RoutingMode get_hierarchy() const { return hierarchy_; }

  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& get_name() const { return name_; }
  /** @brief Retrieves the name of that netzone as a C string */
  const char* get_cname() const { return name_.c_str(); };

  /** @brief Gets the netpoint associated to this netzone */
  kernel::routing::NetPoint* get_netpoint() const { return netpoint_; }

  std::vector<s4u::Host*> get_all_hosts() const;
  int get_host_count() const;

  /** @brief Make a host within that NetZone */
  s4u::Host* create_host(const std::string& name, const std::vector<double>& speed_per_pstate);
  /** @brief Create a disk with the disk model from this NetZone */
  s4u::Disk* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth);
  /** @brief Make a link within that NetZone */
  virtual s4u::Link* create_link(const std::string& name, const std::vector<double>& bandwidths);
  /** @brief Make a router within that NetZone */
  NetPoint* create_router(const std::string& name);
  /** @brief Creates a new route in this NetZone */
  virtual void add_bypass_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                std::vector<resource::LinkImpl*>& link_list, bool symmetrical);

  /** @brief Seal your netzone once you're done adding content, and before routing stuff through it */
  void seal();
  virtual int add_component(kernel::routing::NetPoint* elm); /* A host, a router or a netzone, whatever */
  virtual void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                         kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                         const std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical);
  /** @brief Set parent of this Netzone */
  void set_parent(NetZoneImpl* parent);
  /** @brief Set network model for this Netzone */
  void set_network_model(std::shared_ptr<resource::NetworkModel> netmodel);
  void set_cpu_vm_model(std::shared_ptr<resource::CpuModel> cpu_model);
  void set_cpu_pm_model(std::shared_ptr<resource::CpuModel> cpu_model);
  void set_disk_model(std::shared_ptr<resource::DiskModel> disk_model);
  void set_host_model(std::shared_ptr<surf::HostModel> host_model);

  /* @brief get the route between two nodes in the full platform
   *
   * @param src where from
   * @param dst where to
   * @param links Accumulator in which all traversed links should be pushed (caller must empty it)
   * @param latency Accumulator in which the latencies should be added (caller must set it to 0)
   */
  static void get_global_route(routing::NetPoint* src, routing::NetPoint* dst,
                               /* OUT */ std::vector<resource::LinkImpl*>& links, double* latency);

  virtual void get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                         std::map<std::string, xbt_edge_t, std::less<>>* edges) = 0;

private:
  RoutingMode hierarchy_ = RoutingMode::base;
  std::shared_ptr<resource::NetworkModel> network_model_;
  std::shared_ptr<resource::CpuModel> cpu_model_vm_;
  std::shared_ptr<resource::CpuModel> cpu_model_pm_;
  std::shared_ptr<resource::DiskModel> disk_model_;
  std::shared_ptr<simgrid::surf::HostModel> host_model_;
  /** @brief Perform sealing procedure for derived classes, if necessary */
  virtual void do_seal() { /* obviously nothing to do by default */ }
  void add_child(NetZoneImpl* new_zone);
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_ROUTING_NETZONEIMPL_HPP */
