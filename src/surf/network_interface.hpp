/* Copyright (c) 2004-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

/*********
 * Tools *
 *********/

void net_define_callbacks(void);

/*********
 * Model *
 *********/
/** @ingroup SURF_network_interface
 * @brief SURF network model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class NetworkModel : public Model {
public:
  /**
   * @brief NetworkModel constructor
   */
  NetworkModel() : Model("network") {
  };

  /**
   * @brief NetworkModel constructor
   * 
   * @param name The name of the NetworkModel
   */
  NetworkModel(const char *name) : Model(name) {
	f_networkSolve = lmm_solve;
	m_haveGap = false;
  };

  /**
   * @brief The destructor of the NetworkModel
   */
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
   * @param bw_trace The trace associated to the NetworkLink bandwidth [TODO]
   * @param lat_initial The initial latency of the NetworkLink in seconds
   * @param lat_trace The trace associated to the NetworkLink latency [TODO]
   * @param state_initial The initial NetworkLink (state)[e_surf_resource_state_t]
   * @param state_trace The trace associated to the NetworkLink (state)[e_surf_resource_state_t] [TODO]
   * @param policy The sharing policy of the NetworkLink
   * @param properties Dictionary of properties associated to this Resource
   * @return The created NetworkLink
   */
  virtual NetworkLinkPtr createResource(const char *name,
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
   * @brief Create a communication between two [TODO]
   * @details [TODO]
   * 
   * @param src The source [TODO]
   * @param dst The destination [TODO]
   * @param size The size of the communication in bytes
   * @param rate The 
   * @return The action representing the communication
   */
  virtual ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate)=0;

  /**
   * @brief Get the route between two RoutingEdge
   * @details [TODO]
   * 
   * @param src [TODO]
   * @param dst [TODO]
   * 
   * @return A xbt_dynar_t of [TODO]
   */
  virtual xbt_dynar_t getRoute(RoutingEdgePtr src, RoutingEdgePtr dst); //FIXME: kill field? That is done by the routing nowadays

  /**
   * @brief Function pointer to the function to use to solve the lmm_system_t
   * 
   * @param system The lmm_system_t to solve
   */
  void (*f_networkSolve)(lmm_system_t);

  /**
   * @brief [brief description]
   * @details [long description]
   * 
   * @param size [description]
   * @return [description]
   */
  virtual double latencyFactor(double size);

  /**
   * @brief [brief description]
   * @details [long description]
   * 
   * @param size [description]
   * @return [description]
   */
  virtual double bandwidthFactor(double size);

  /**
   * @brief [brief description]
   * @details [long description]
   * 
   * @param rate [description]
   * @param bound [description]
   * @param size [description]
   * @return [description]
   */
  virtual double bandwidthConstraint(double rate, double bound, double size);
  bool m_haveGap;
};

/************
 * Resource *
 ************/
 /** @ingroup SURF_network_interface
  * @brief SURF network link interface class
  * @details A NetworkLink represent the link between two [Workstations](\ref Workstation)
  */
class NetworkLink : public Resource {
public:
  /**
   * @brief NetworkLink constructor
   * 
   * @param model The CpuModel associated to this NetworkLink
   * @param name The name of the NetworkLink
   * @param props Dictionary of properties associated to this NetworkLink
   */
  NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props);

  /**
   * @brief NetworkLink constructor
   * 
   * @param model The CpuModel associated to this NetworkLink
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

  /**
   * @brief Get the bandwidth in bytes per second of current NetworkLink
   * 
   * @return The bandwith in bytes per second of the current NetworkLink
   */
  virtual double getBandwidth();

  /**
   * @brief Get the latency in seconds of current NetworkLink
   * 
   * @return The latency in seconds of the current NetworkLink
   */
  virtual double getLatency();

  /**
   * @brief Check if the NetworkLink is shared
   * @details [long description]
   * 
   * @return true if the current NetwokrLink is shared, false otherwise
   */
  virtual bool isShared();

  /**
   * @brief Check if the NetworkLink is used
   * 
   * @return true if the current NetwokrLink is used, false otherwise
   */
  bool isUsed();

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
 * @details A NetworkAction represent a communication bettween two [Workstations](\ref Workstation)
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
   * @param var The lmm variable associated to this Action if it is part of a LMM component
   */
  NetworkAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {};

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


