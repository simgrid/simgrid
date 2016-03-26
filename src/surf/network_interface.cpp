/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "network_interface.hpp"
#include "simgrid/sg_config.h"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf, "Logging specific to the SURF network module");

/*********
 * C API *
 *********/

extern "C" {

  const char* sg_link_name(Link *link) {
    return link->getName();
  }
  Link * sg_link_by_name(const char* name) {
    return Link::byName(name);
  }

  int sg_link_is_shared(Link *link){
    return link->sharingPolicy();
  }
  double sg_link_bandwidth(Link *link){
    return link->getBandwidth();
  }
  double sg_link_latency(Link *link){
    return link->getLatency();
  }
  void* sg_link_data(Link *link) {
    return link->getData();
  }
  void sg_link_data_set(Link *link,void *data) {
    link->setData(data);
  }
  int sg_link_count(void) {
    return Link::linksCount();
  }
  Link** sg_link_list(void) {
    return Link::linksList();
  }
  void sg_link_exit(void) {
    Link::linksExit();
  }

}

/*****************
 * List of links *
 *****************/

namespace simgrid {
  namespace surf {

    boost::unordered_map<std::string,Link *> *Link::links = new boost::unordered_map<std::string,Link *>();
    Link *Link::byName(const char* name) {
      if (links->find(name) == links->end())
        return NULL;
      return  links->at(name);
    }
    /** @brief Returns the amount of links in the platform */
    int Link::linksCount() {
      return links->size();
    }
    /** @brief Returns a list of all existing links */
    Link **Link::linksList() {
      Link **res = xbt_new(Link*, (int)links->size());
      int i=0;
      for (auto kv : *links) {
        res[i++] = kv.second;
      }
      return res;
    }
    /** @brief destructor of the static data */
    void Link::linksExit() {
      for (auto kv : *links)
        (kv.second)->destroy();
      delete links;
    }

    /*************
     * Callbacks *
     *************/

    simgrid::xbt::signal<void(simgrid::surf::Link*)> Link::onCreation;
    simgrid::xbt::signal<void(simgrid::surf::Link*)> Link::onDestruction;
    simgrid::xbt::signal<void(simgrid::surf::Link*)> Link::onStateChange;

    simgrid::xbt::signal<void(simgrid::surf::NetworkAction*, simgrid::surf::Action::State, simgrid::surf::Action::State)> networkActionStateChangedCallbacks;
    simgrid::xbt::signal<void(simgrid::surf::NetworkAction*, simgrid::surf::NetCard *src, simgrid::surf::NetCard *dst)> Link::onCommunicate;

  }
}

/*********
 * Model *
 *********/

simgrid::surf::NetworkModel *surf_network_model = NULL;

namespace simgrid {
  namespace surf {

    double NetworkModel::latencyFactor(double /*size*/) {
      return sg_latency_factor;
    }

    double NetworkModel::bandwidthFactor(double /*size*/) {
      return sg_bandwidth_factor;
    }

    double NetworkModel::bandwidthConstraint(double rate, double /*bound*/, double /*size*/) {
      return rate;
    }

    double NetworkModel::next_occuring_event_full(double now)
    {
      NetworkAction *action = NULL;
      ActionList *runningActions = surf_network_model->getRunningActionSet();
      double minRes;

      minRes = shareResourcesMaxMin(runningActions, surf_network_model->maxminSystem_, surf_network_model->f_networkSolve);

      for(ActionList::iterator it(runningActions->begin()), itend(runningActions->end())
          ; it != itend ; ++it) {
        action = static_cast<NetworkAction*>(&*it);
        if (action->latency_ > 0) {
          minRes = (minRes < 0) ? action->latency_ : std::min(minRes, action->latency_);
        }
      }

      XBT_DEBUG("Min of share resources %f", minRes);

      return minRes;
    }

    /************
     * Resource *
     ************/

    Link::Link(simgrid::surf::NetworkModel *model, const char *name, xbt_dict_t props)
    : Resource(model, name),
      PropertyHolder(props)
    {
      links->insert({name, this});

      m_latency.scale = 1;
      m_bandwidth.scale = 1;
      XBT_DEBUG("Create link '%s'",name);
    }

    Link::Link(simgrid::surf::NetworkModel *model, const char *name, xbt_dict_t props, lmm_constraint_t constraint)
    : Resource(model, name, constraint),
      PropertyHolder(props)
    {
      if (strcmp(name,"__loopback__"))
        xbt_assert(!Link::byName(name), "Link '%s' declared several times in the platform.", name);

      m_latency.scale = 1;
      m_bandwidth.scale = 1;

      links->insert({name, this});
      XBT_DEBUG("Create link '%s'",name);

    }

    /** @brief use destroy() instead of this destructor */
    Link::~Link() {
      xbt_assert(currentlyDestroying_, "Don't delete Links directly. Call destroy() instead.");
    }
    /** @brief Fire the require callbacks and destroy the object
     *
     * Don't delete directly an Link, call l->destroy() instead.
     */
    void Link::destroy()
    {
      if (!currentlyDestroying_) {
        currentlyDestroying_ = true;
        onDestruction(this);
        delete this;
      }
    }

    bool Link::isUsed()
    {
      return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
    }

    double Link::getLatency()
    {
      return m_latency.peak * m_latency.scale;
    }

    double Link::getBandwidth()
    {
      return m_bandwidth.peak * m_bandwidth.scale;
    }

    int Link::sharingPolicy()
    {
      return lmm_constraint_sharing_policy(getConstraint());
    }

    void Link::turnOn(){
      if (isOff()) {
        Resource::turnOn();
        onStateChange(this);
      }
    }
    void Link::turnOff(){
      if (isOn()) {
        Resource::turnOff();
        onStateChange(this);
      }
    }
    void Link::setStateTrace(tmgr_trace_t trace) {
      xbt_assert(m_stateEvent==NULL,"Cannot set a second state trace to Link %s", getName());
      m_stateEvent = future_evt_set->add_trace(trace, 0.0, this);
    }
    void Link::setBandwidthTrace(tmgr_trace_t trace)
    {
      xbt_assert(m_bandwidth.event==NULL,"Cannot set a second bandwidth trace to Link %s", getName());
      m_bandwidth.event = future_evt_set->add_trace(trace, 0.0, this);
    }
    void Link::setLatencyTrace(tmgr_trace_t trace)
    {
      xbt_assert(m_latency.event==NULL,"Cannot set a second latency trace to Link %s", getName());
      m_latency.event = future_evt_set->add_trace(trace, 0.0, this);
    }


    /**********
     * Action *
     **********/

    void NetworkAction::setState(Action::State state){
      Action::State old = getState();
      Action::setState(state);
      networkActionStateChangedCallbacks(this, old, state);
    }

  }
}

#endif /* NETWORK_INTERFACE_CPP_ */
