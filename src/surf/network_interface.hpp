/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_INTERFACE_HPP_
#define SURF_NETWORK_INTERFACE_HPP_

#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/NetworkModelIntf.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include <xbt/PropertyHolder.hpp>

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
class NetworkModel : public Model, public NetworkModelIntf {
public:
  static config::Flag<double> cfg_tcp_gamma;
  static config::Flag<bool> cfg_crosstraffic;

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
  virtual LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths) = 0;

  virtual LinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths) = 0;

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
  virtual double get_latency_factor(double /* size */) { return sg_latency_factor; }

  /**
   * @brief Get the right multiplicative factor for the bandwidth.
   * @details Depending on the model, the effective bandwidth when sending a message might be different from the
   * theoretical bandwidth of the link, in function of the message size. In order to account for this, this function
   * gets this factor.
   *
   * @param size The size of the message.
   * @return The bandwidth factor.
   */
  virtual double get_bandwidth_factor(double /* size*/) { return sg_bandwidth_factor; }

  double next_occurring_event_full(double now) override;

  void set_lat_factor_cb(const std::function<NetworkFactorCb>& cb) override { THROW_UNIMPLEMENTED; }
  void set_bw_factor_cb(const std::function<NetworkFactorCb>& cb) override { THROW_UNIMPLEMENTED; }

  LinkImpl* loopback_ = nullptr;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_network_interface
 * @brief SURF network link interface class
 * @details A Link represents the link between two [hosts](@ref simgrid::surf::HostImpl)
 */
class LinkImpl : public Resource_T<LinkImpl>, public xbt::PropertyHolder {
  s4u::Link piface_;

protected:
  explicit LinkImpl(const std::string& name);
  LinkImpl(const LinkImpl&) = delete;
  LinkImpl& operator=(const LinkImpl&) = delete;
  ~LinkImpl() override                 = default; // Use destroy() instead of this destructor.

public:
  void destroy(); // Must be called instead of the destructor

  void latency_check(double latency) const;

  /** @brief Public interface */
  const s4u::Link* get_iface() const { return &piface_; }
  s4u::Link* get_iface() { return &piface_; }

  /** @brief Get the bandwidth in bytes per second of current Link */
  double get_bandwidth() const { return bandwidth_.peak * bandwidth_.scale; }
  /** @brief Update the bandwidth in bytes per second of current Link */
  virtual void set_bandwidth(double value) = 0;

  /** @brief Get the latency in seconds of current Link */
  double get_latency() const { return latency_.peak * latency_.scale; }
  /** @brief Update the latency in seconds of current Link */
  virtual LinkImpl* set_latency(double value) = 0;

  /** @brief The sharing policy */
  virtual LinkImpl* set_sharing_policy(s4u::Link::SharingPolicy policy);
  virtual s4u::Link::SharingPolicy get_sharing_policy() const;

  /** @brief Check if the Link is used */
  bool is_used() const override;

  void turn_on() override;
  void turn_off() override;

  void seal() override;

  void on_bandwidth_change() const;

  /* setup the profile file with bandwidth events (peak speed changes due to external load).
   * Profile must contain percentages (value between 0 and 1). */
  virtual LinkImpl* set_bandwidth_profile(kernel::profile::Profile* profile);
  /* setup the profile file with latency events (peak latency changes due to external load).
   * Profile must contain absolute values */
  virtual LinkImpl* set_latency_profile(kernel::profile::Profile* profile);

  void set_concurrency_limit(int limit) const;

  Metric latency_   = {0.0, 1, nullptr};
  Metric bandwidth_ = {1.0, 1, nullptr};
};

/**********
 * Action *
 **********/
/** @ingroup SURF_network_interface
 * @brief SURF network action interface class
 * @details A NetworkAction represents a communication between two [hosts](@ref simgrid::surf::HostImpl)
 */
class NetworkAction : public Action {
  s4u::Host& src_;
  s4u::Host& dst_;

public:
  /** @brief Constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
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
  virtual std::list<LinkImpl*> get_links() const;

  double latency_         = 0.; // Delay before the action starts
  double lat_current_     = 0.; // Used to compute the communication RTT, and accordingly limit the communication rate
  double sharing_penalty_ = {};

  s4u::Host& get_src() const { return src_; }
  s4u::Host& get_dst() const { return dst_; }
};

/* Insert link(s) at the end of vector `result' (at the beginning, and reversed, for insert_link_latency()), and add
 * link->get_latency() to *latency when latency is not null
 */
void add_link_latency(std::vector<LinkImpl*>& result, LinkImpl* link, double* latency);
void add_link_latency(std::vector<LinkImpl*>& result, const std::vector<LinkImpl*>& links, double* latency);
void insert_link_latency(std::vector<LinkImpl*>& result, const std::vector<LinkImpl*>& links, double* latency);

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SURF_NETWORK_INTERFACE_HPP_ */
