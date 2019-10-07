/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_INTERFACE_HPP_
#define SURF_NETWORK_INTERFACE_HPP_

#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/PropertyHolder.hpp"

#include <list>
#include <unordered_map>

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {
/*********
 * Model *
 *********/

/** @ingroup SURF_network_interface
 * @brief SURF network model interface class
 * @details A model is an object which handles the interactions between its Resources and its Actions
 */
class NetworkModel : public Model {
public:
  static config::Flag<double> cfg_tcp_gamma;
  static config::Flag<bool> cfg_crosstraffic;

  explicit NetworkModel(Model::UpdateAlgo algo) : Model(algo) {}
  NetworkModel(const NetworkModel&) = delete;
  NetworkModel& operator=(const NetworkModel&) = delete;
  ~NetworkModel() override;

  /**
   * @brief Create a Link
   *
   * @param name The name of the Link
   * @param bandwidth The initial bandwidth of the Link in bytes per second
   * @param latency The initial latency of the Link in seconds
   * @param policy The sharing policy of the Link
   */
  virtual LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                                s4u::Link::SharingPolicy policy) = 0;

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
  virtual Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) = 0;

  /**
   * @brief Get the right multiplicative factor for the latency.
   * @details Depending on the model, the effective latency when sending a message might be different from the
   * theoretical latency of the link, in function of the message size. In order to account for this, this function gets
   * this factor.
   *
   * @param size The size of the message.
   * @return The latency factor.
   */
  virtual double get_latency_factor(double size);

  /**
   * @brief Get the right multiplicative factor for the bandwidth.
   * @details Depending on the model, the effective bandwidth when sending a message might be different from the
   * theoretical bandwidth of the link, in function of the message size. In order to account for this, this function
   * gets this factor.
   *
   * @param size The size of the message.
   * @return The bandwidth factor.
   */
  virtual double get_bandwidth_factor(double size);

  /**
   * @brief Get definitive bandwidth.
   * @details It gives the minimum bandwidth between the one that would occur if no limitation was enforced, and the
   * one arbitrary limited.
   * @param rate The desired maximum bandwidth.
   * @param bound The bandwidth with only the network taken into account.
   * @param size The size of the message.
   * @return The new bandwidth.
   */
  virtual double get_bandwidth_constraint(double rate, double bound, double size);
  double next_occuring_event_full(double now) override;

  LinkImpl* loopback_ = nullptr;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_network_interface
 * @brief SURF network link interface class
 * @details A Link represents the link between two [hosts](@ref simgrid::surf::HostImpl)
 */
class LinkImpl : public Resource, public surf::PropertyHolder {
  bool currently_destroying_ = false;

protected:
  LinkImpl(NetworkModel* model, const std::string& name, lmm::Constraint* constraint);
  LinkImpl(const LinkImpl&) = delete;
  LinkImpl& operator=(const LinkImpl&) = delete;
  ~LinkImpl() override;

public:
  void destroy(); // Must be called instead of the destructor

  /** @brief Public interface */
  s4u::Link piface_;

  /** @brief Get the bandwidth in bytes per second of current Link */
  virtual double get_bandwidth();

  /** @brief Update the bandwidth in bytes per second of current Link */
  virtual void set_bandwidth(double value) = 0;

  /** @brief Get the latency in seconds of current Link */
  virtual double get_latency();

  /** @brief Update the latency in seconds of current Link */
  virtual void set_latency(double value) = 0;

  /** @brief The sharing policy */
  virtual s4u::Link::SharingPolicy get_sharing_policy();

  /** @brief Check if the Link is used */
  bool is_used() override;

  void turn_on() override;
  void turn_off() override;

  void on_bandwidth_change();

  virtual void
  set_bandwidth_profile(kernel::profile::Profile* profile); /*< setup the profile file with bandwidth events
                                                   (peak speed changes due to external load). Trace must
                                                   contain percentages (value between 0 and 1). */
  virtual void
  set_latency_profile(kernel::profile::Profile* profile); /*< setup the trace file with latency events (peak
                                                 latency changes due to external load).   Trace must contain
                                                 absolute values */

  Metric latency_                   = {1.0, 0, nullptr};
  Metric bandwidth_                 = {1.0, 0, nullptr};

};

/**********
 * Action *
 **********/
/** @ingroup SURF_network_interface
 * @brief SURF network action interface class
 * @details A NetworkAction represents a communication between two [hosts](@ref simgrid::surf::HostImpl)
 */
class NetworkAction : public Action {
public:
  /** @brief Constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   */
  NetworkAction(Model* model, double cost, bool failed) : Action(model, cost, failed) {}

  /**
   * @brief NetworkAction constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in bytes
   * @param failed Actions can be created in a failed state
   * @param var The lmm variable associated to this Action if it is part of a LMM component
   */
  NetworkAction(Model* model, double cost, bool failed, lmm::Variable* var) : Action(model, cost, failed, var){};

  void set_state(Action::State state) override;
  virtual std::list<LinkImpl*> links() const;

  double latency_    = {};
  double lat_current_ = {};
  double sharing_penalty_ = {};
  double rate_       = {};
};
} // namespace resource
} // namespace kernel
} // namespace simgrid

/** @ingroup SURF_models
 *  @brief The network model
 */
XBT_PUBLIC_DATA simgrid::kernel::resource::NetworkModel* surf_network_model;

#endif /* SURF_NETWORK_INTERFACE_HPP_ */


