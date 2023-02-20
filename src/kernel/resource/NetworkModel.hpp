/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_NETWORKMODEL_HPP
#define SIMGRID_KERNEL_RESOURCE_NETWORKMODEL_HPP

#include "simgrid/kernel/resource/Model.hpp"
#include "src/kernel/resource/NetworkModelFactors.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

#include <list>

namespace simgrid::kernel::resource {

/*********
 * Model *
 *********/

/** @ingroup Model_network_interface
 * @brief Network model interface class
 * @details A model is an object which handles the interactions between its Resources and its Actions
 */
class NetworkModel : public Model, public NetworkModelFactors {
public:
  static config::Flag<double> cfg_tcp_gamma;
  static config::Flag<bool> cfg_crosstraffic;
  static config::Flag<double> cfg_weight_S_parameter;

  using Model::Model;
  NetworkModel(const NetworkModel&) = delete;
  NetworkModel& operator=(const NetworkModel&) = delete;
  ~NetworkModel() override;

  /**
   * @brief Create a [WiFi]Link
   *
   * @param name The name of the Link
   * @param bandwidths The vector of bandwidths of the Link in bytes per second
   */
  virtual StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths) = 0;

  virtual StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths) = 0;

  /**
   * @brief Create a communication between two hosts.
   * @details It makes calls to the routing part, and execute the communication between the two end points.
   *
   * @param src The source of the communication
   * @param dst The destination of the communication
   * @param size The size of the communication in bytes
   * @param rate Allows to limit the transfer rate. Negative value means unlimited.
   * @return The action representing the communication
   */
  virtual Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed) = 0;

  double next_occurring_event_full(double now) override;

  std::unique_ptr<StandardLinkImpl, StandardLinkImpl::Deleter> loopback_;
};

/**********
 * Action *
 **********/
/** @ingroup Model_network_interface
 * @brief Network action interface class
 * @details A NetworkAction represents a communication between two [hosts](@ref HostImpl)
 */
class NetworkAction : public Action {
  s4u::Host& src_;
  s4u::Host& dst_;

public:
  /** @brief Constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in bytes
   * @param failed Actions can be created in a failed state
   */
  NetworkAction(Model* model, s4u::Host& src, s4u::Host& dst, double cost, bool failed)
      : Action(model, cost, failed), src_(src), dst_(dst)
  {
  }

  /**
   * @brief NetworkAction constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in bytes
   * @param failed Actions can be created in a failed state
   * @param var The lmm variable associated to this Action if it is part of a LMM component
   */
  NetworkAction(Model* model, s4u::Host& src, s4u::Host& dst, double cost, bool failed, lmm::Variable* var)
      : Action(model, cost, failed, var), src_(src), dst_(dst){};

  void set_state(Action::State state) override;
  virtual std::list<StandardLinkImpl*> get_links() const;

  double latency_         = 0.; // Delay before the action starts
  double lat_current_     = 0.; // Used to compute the communication RTT, and accordingly limit the communication rate
  double sharing_penalty_ = {};

  s4u::Host& get_src() const { return src_; }
  s4u::Host& get_dst() const { return dst_; }
};

/* Insert link(s) at the end of vector `result' (at the beginning, and reversed, for insert_link_latency()), and add
 * link->get_latency() to *latency when latency is not null
 */
void add_link_latency(std::vector<StandardLinkImpl*>& result, StandardLinkImpl* link, double* latency);
void add_link_latency(std::vector<StandardLinkImpl*>& result, const std::vector<StandardLinkImpl*>& links,
                      double* latency);
void insert_link_latency(std::vector<StandardLinkImpl*>& result, const std::vector<StandardLinkImpl*>& links,
                         double* latency);

} // namespace simgrid::kernel::resource

#endif /* SIMGRID_KERNEL_RESOURCE_NETWORKMODEL_HPP */
