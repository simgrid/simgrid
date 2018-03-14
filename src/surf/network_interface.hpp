/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_INTERFACE_HPP_
#define SURF_NETWORK_INTERFACE_HPP_

#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/PropertyHolder.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/base.h"

#include <list>
#include <unordered_map>

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {
/*********
 * Model *
 *********/

/** @ingroup SURF_network_interface
 * @brief SURF network model interface class
 * @details A model is an object which handles the interactions between its Resources and its Actions
 */
class NetworkModel : public kernel::resource::Model {
public:
  /** @brief Constructor */
  NetworkModel() : Model() {}

  /** @brief Destructor */
  ~NetworkModel() override;

  /**
   * @brief Create a Link
   *
   * @param name The name of the Link
   * @param bandwidth The initial bandwidth of the Link in bytes per second
   * @param latency The initial latency of the Link in seconds
   * @param policy The sharing policy of the Link
   */
  virtual LinkImpl* createLink(const std::string& name, double bandwidth, double latency,
                               e_surf_link_sharing_policy_t policy) = 0;

  /**
   * @brief Create a communication between two hosts.
   * @details It makes calls to the routing part, and execute the communication
   *          between the two end points.
   *
   * @param src The source of the communication
   * @param dst The destination of the communication
   * @param size The size of the communication in bytes
   * @param rate Allows to limit the transfer rate. Negative value means
   * unlimited.
   * @return The action representing the communication
   */
  virtual kernel::resource::Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) = 0;

  /** @brief Function pointer to the function to use to solve the lmm_system_t
   *
   * @param system The lmm_system_t to solve
   */
  void (*f_networkSolve)(lmm_system_t) = simgrid::kernel::lmm::lmm_solve;

  /**
   * @brief Get the right multiplicative factor for the latency.
   * @details Depending on the model, the effective latency when sending
   * a message might be different from the theoretical latency of the link,
   * in function of the message size. In order to account for this, this
   * function gets this factor.
   *
   * @param size The size of the message.
   * @return The latency factor.
   */
  virtual double latencyFactor(double size);

  /**
   * @brief Get the right multiplicative factor for the bandwidth.
   * @details Depending on the model, the effective bandwidth when sending
   * a message might be different from the theoretical bandwidth of the link,
   * in function of the message size. In order to account for this, this
   * function gets this factor.
   *
   * @param size The size of the message.
   * @return The bandwidth factor.
   */
  virtual double bandwidthFactor(double size);

  /**
   * @brief Get definitive bandwidth.
   * @details It gives the minimum bandwidth between the one that would
   * occur if no limitation was enforced, and the one arbitrary limited.
   * @param rate The desired maximum bandwidth.
   * @param bound The bandwidth with only the network taken into account.
   * @param size The size of the message.
   * @return The new bandwidth.
   */
  virtual double bandwidthConstraint(double rate, double bound, double size);
  double nextOccuringEventFull(double now) override;

  LinkImpl* loopback_ = nullptr;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_network_interface
 * @brief SURF network link interface class
 * @details A Link represents the link between two [hosts](\ref simgrid::surf::HostImpl)
 */
class LinkImpl : public simgrid::kernel::resource::Resource, public simgrid::surf::PropertyHolder {
protected:
  LinkImpl(simgrid::surf::NetworkModel* model, const std::string& name, kernel::lmm::Constraint* constraint);
  ~LinkImpl() override;

public:
  void destroy(); // Must be called instead of the destructor
private:
  bool currentlyDestroying_ = false;

public:
  /** @brief Public interface */
  s4u::Link piface_;

  /** @brief Get the bandwidth in bytes per second of current Link */
  virtual double bandwidth();

  /** @brief Update the bandwidth in bytes per second of current Link */
  virtual void setBandwidth(double value) = 0;

  /** @brief Get the latency in seconds of current Link */
  virtual double latency();

  /** @brief Update the latency in seconds of current Link */
  virtual void setLatency(double value) = 0;

  /** @brief The sharing policy is a @{link e_surf_link_sharing_policy_t::EType} (0: FATPIPE, 1: SHARED, 2:
   * SPLITDUPLEX) */
  virtual int sharingPolicy();

  /** @brief Check if the Link is used */
  bool isUsed() override;

  void turnOn() override;
  void turnOff() override;

  virtual void setStateTrace(tmgr_trace_t trace); /*< setup the trace file with states events (ON or OFF).
                                                          Trace must contain boolean values. */
  virtual void setBandwidthTrace(
      tmgr_trace_t trace); /*< setup the trace file with bandwidth events (peak speed changes due to external load).
                                   Trace must contain percentages (value between 0 and 1). */
  virtual void setLatencyTrace(
      tmgr_trace_t trace); /*< setup the trace file with latency events (peak latency changes due to external load).
                                   Trace must contain absolute values */

  tmgr_trace_event_t stateEvent_    = nullptr;
  Metric latency_                   = {1.0, 0, nullptr};
  Metric bandwidth_                 = {1.0, 0, nullptr};

  /* User data */
  void* getData() { return userData; }
  void setData(void* d) { userData = d; }
private:
  void* userData = nullptr;

  /* List of all links. FIXME: should move to the Engine */
  static std::unordered_map<std::string, LinkImpl*>* links;

public:
  static LinkImpl* byName(std::string name);
  static int linksCount();
  static LinkImpl** linksList();
  static void linksList(std::vector<s4u::Link*>* linkList);
  static void linksExit();
};

/**********
 * Action *
 **********/
/** @ingroup SURF_network_interface
 * @brief SURF network action interface class
 * @details A NetworkAction represents a communication between two [hosts](\ref HostImpl)
 */
class NetworkAction : public simgrid::kernel::resource::Action {
public:
  /** @brief Constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   */
  NetworkAction(simgrid::kernel::resource::Model* model, double cost, bool failed)
      : simgrid::kernel::resource::Action(model, cost, failed)
  {
  }

  /**
   * @brief NetworkAction constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this Action if it is part of a LMM component
   */
  NetworkAction(simgrid::kernel::resource::Model* model, double cost, bool failed, kernel::lmm::Variable* var)
      : simgrid::kernel::resource::Action(model, cost, failed, var){};

  void setState(simgrid::kernel::resource::Action::State state) override;
  virtual std::list<LinkImpl*> links();

  double latency_    = {};
  double latCurrent_ = {};
  double weight_     = {};
  double rate_       = {};
};
}
}

#endif /* SURF_NETWORK_INTERFACE_HPP_ */


