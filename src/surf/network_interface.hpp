/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "xbt/fifo.h"
#include "surf_interface.hpp"
#include "surf_routing.hpp"

#ifndef SURF_NETWORK_INTERFACE_HPP_
#define SURF_NETWORK_INTERFACE_HPP_

/***********
 * Classes *
 ***********/
class NetworkModel;
typedef NetworkModel *NetworkModelPtr;

class NetworkLink;
typedef NetworkLink *NetworkLinkPtr;

class NetworkAction;
typedef NetworkAction *NetworkActionPtr;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emits the callbacks after NetworkLink creation
 * @details Callback functions have the following signature: `void(NetworkLinkPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, NetworkLinkPtr)) networkLinkCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emits the callbacks after NetworkLink destruction
 * @details Callback functions have the following signature: `void(NetworkLinkPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, NetworkLinkPtr)) networkLinkDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emits the callbacks after NetworkLink State changed
 * @details Callback functions have the following signature: `void(NetworkLinkActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, NetworkLinkPtr, e_surf_resource_state_t, e_surf_resource_state_t)) networkLinkStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emits the callbacks after NetworkAction State changed
 * @details Callback functions have the following signature: `void(NetworkActionPtr action, e_surf_action_state_t old, e_surf_action_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, NetworkActionPtr, e_surf_action_state_t, e_surf_action_state_t)) networkActionStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emits the callbacks after communication created
 * @details Callback functions have the following signature: `void(NetworkActionPtr action, RoutingEdgePtr src, RoutingEdgePtr dst, double size, double rate)`
 */
XBT_PUBLIC_DATA( surf_callback(void, NetworkActionPtr, RoutingEdgePtr src, RoutingEdgePtr dst, double size, double rate)) networkCommunicateCallbacks;

/*********
 * Tools *
 *********/
XBT_PUBLIC(void) netlink_parse_init(sg_platf_link_cbarg_t link);

XBT_PUBLIC(void) net_add_traces();

/*********
 * Model *
 *********/
/** @ingroup SURF_network_interface
 * @brief SURF network model interface class
 * @details A model is an object which handles the interactions between its Resources and its Actions
 */
class NetworkModel : public Model {
public:
  /** @brief NetworkModel constructor */
  NetworkModel() : Model("network") {
    f_networkSolve = lmm_solve;
  };

  /** @brief NetworkModel constructor */
  NetworkModel(const char *name) : Model(name) {
	f_networkSolve = lmm_solve;
  };

  /** @brief The destructor of the NetworkModel */
  ~NetworkModel() {
	if (p_maxminSystem)
	  lmm_system_free(p_maxminSystem);
	if (p_actionHeap)
	  xbt_heap_free(p_actionHeap);
	if (p_modifiedSet)
	  delete p_modifiedSet;
  }

  /**
   * @brief Create a NetworkLink
   *
   * @param name The name of the NetworkLink
   * @param bw_initial The initial bandwidth of the NetworkLink in bytes per second
   * @param bw_trace The trace associated to the NetworkLink bandwidth
   * @param lat_initial The initial latency of the NetworkLink in seconds
   * @param lat_trace The trace associated to the NetworkLink latency
   * @param state_initial The initial NetworkLink (state)[e_surf_resource_state_t]
   * @param state_trace The trace associated to the NetworkLink (state)[e_surf_resource_state_t]
   * @param policy The sharing policy of the NetworkLink
   * @param properties Dictionary of properties associated to this Resource
   * @return The created NetworkLink
   */
  virtual NetworkLinkPtr createNetworkLink(const char *name,
                                   double bw_initial,
                                   tmgr_trace_t bw_trace,
                                   double lat_initial,
                                   tmgr_trace_t lat_trace,
                                   e_surf_resource_state_t state_initial,
                                   tmgr_trace_t state_trace,
                                   e_surf_link_sharing_policy_t policy,
                                   xbt_dict_t properties)=0;

  virtual void gapAppend(double /*size*/, const NetworkLinkPtr /*link*/, NetworkActionPtr /*action*/) {};

  /**
   * @brief Create a communication between two hosts.
   * @details It makes calls to the routing part, and execute the communication
   * between the two end points.
   *
   * @param src The source of the communication
   * @param dst The destination of the communication
   * @param size The size of the communication in bytes
   * @param rate Allows to limit the transfer rate. Negative value means
   * unlimited.
   * @return The action representing the communication
   */
  virtual ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate)=0;

  /**
   * @brief Function pointer to the function to use to solve the lmm_system_t
   *
   * @param system The lmm_system_t to solve
   */
  void (*f_networkSolve)(lmm_system_t);

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
  double shareResourcesFull(double now);
  bool m_haveGap = false;
};

/************
 * Resource *
 ************/
 /** @ingroup SURF_network_interface
  * @brief SURF network link interface class
  * @details A NetworkLink represents the link between two [hosts](\ref Host)
  */
class NetworkLink : public Resource {
public:
  /**
   * @brief NetworkLink constructor
   *
   * @param model The NetworkModel associated to this NetworkLink
   * @param name The name of the NetworkLink
   * @param props Dictionary of properties associated to this NetworkLink
   */
  NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props);

  /**
   * @brief NetworkLink constructor
   *
   * @param model The NetworkModel associated to this NetworkLink
   * @param name The name of the NetworkLink
   * @param props Dictionary of properties associated to this NetworkLink
   * @param constraint The lmm constraint associated to this Cpu if it is part of a LMM component
   * @param history [TODO]
   * @param state_trace [TODO]
   */
  NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props,
              lmm_constraint_t constraint,
              tmgr_history_t history,
              tmgr_trace_t state_trace);

  /** @brief NetworkLink destructor */
  ~NetworkLink();

  /** @brief Get the bandwidth in bytes per second of current NetworkLink */
  virtual double getBandwidth();

  /** @brief Update the bandwidth in bytes per second of current NetworkLink */
  virtual void updateBandwidth(double value, double date=surf_get_clock())=0;

  /** @brief Get the latency in seconds of current NetworkLink */
  virtual double getLatency();

  /** @brief Update the latency in seconds of current NetworkLink */
  virtual void updateLatency(double value, double date=surf_get_clock())=0;

  /**
   * @brief Check if the NetworkLink is shared
   *
   * @return true if the current NetwokrLink is shared, false otherwise
   */
  virtual bool isShared();

  /** @brief Check if the NetworkLink is used */
  bool isUsed();

  void setState(e_surf_resource_state_t state);

  /* Using this object with the public part of
    model does not make sense */
  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;

  /* LMM */
  tmgr_trace_event_t p_stateEvent;
  s_surf_metric_t p_power;
};

/**********
 * Action *
 **********/
/** @ingroup SURF_network_interface
 * @brief SURF network action interface class
 * @details A NetworkAction represents a communication between two [hosts](\ref Host)
 */
class NetworkAction : public Action {
public:
  /**
   * @brief NetworkAction constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   */
  NetworkAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {}

  /**
   * @brief NetworkAction constructor
   *
   * @param model The NetworkModel associated to this NetworkAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this Action if it is part of a
   * LMM component
   */
  NetworkAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {};

  void setState(e_surf_action_state_t state);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  /**
   * @brief Check if the action is limited by latency.
   *
   * @return 1 if action is limited by latency, 0 otherwise
   */
  virtual int getLatencyLimited() {return m_latencyLimited;}
#endif

  double m_latency;
  double m_latCurrent;
  double m_weight;
  double m_rate;
  const char* p_senderLinkName;
  double m_senderGap;
  double m_senderSize;
  xbt_fifo_item_t p_senderFifoItem;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int m_latencyLimited;
#endif

};

#endif /* SURF_NETWORK_INTERFACE_HPP_ */


