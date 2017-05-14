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

namespace simgrid {
  namespace surf {

  /* List of links */
  std::unordered_map<std::string, LinkImpl*>* LinkImpl::links = new std::unordered_map<std::string, LinkImpl*>();

  LinkImpl* LinkImpl::byName(const char* name)
  {
    if (links->find(name) == links->end())
      return nullptr;
    return links->at(name);
  }
  /** @brief Returns the amount of links in the platform */
  int LinkImpl::linksCount()
  {
    return links->size();
    }
    /** @brief Returns a list of all existing links */
    LinkImpl** LinkImpl::linksList()
    {
      LinkImpl** res = xbt_new(LinkImpl*, (int)links->size());
      int i          = 0;
      for (auto kv : *links) {
        res[i] = kv.second;
        i++;
      }
      return res;
    }
    /** @brief destructor of the static data */
    void LinkImpl::linksExit()
    {
      for (auto kv : *links)
        (kv.second)->destroy();
      delete links;
    }
  }
}

/*********
 * Model *
 *********/

simgrid::surf::NetworkModel *surf_network_model = nullptr;

namespace simgrid {
  namespace surf {

    NetworkModel::~NetworkModel()
    {
      lmm_system_free(maxminSystem_);
      xbt_heap_free(actionHeap_);
      delete modifiedSet_;
    }

    double NetworkModel::latencyFactor(double /*size*/) {
      return sg_latency_factor;
    }

    double NetworkModel::bandwidthFactor(double /*size*/) {
      return sg_bandwidth_factor;
    }

    double NetworkModel::bandwidthConstraint(double rate, double /*bound*/, double /*size*/) {
      return rate;
    }

    double NetworkModel::nextOccuringEventFull(double now)
    {
      double minRes = Model::nextOccuringEventFull(now);

      for(auto it(getRunningActionSet()->begin()), itend(getRunningActionSet()->end()); it != itend ; it++) {
        NetworkAction *action = static_cast<NetworkAction*>(&*it);
        if (action->latency_ > 0)
          minRes = (minRes < 0) ? action->latency_ : std::min(minRes, action->latency_);
      }

      XBT_DEBUG("Min of share resources %f", minRes);

      return minRes;
    }

    /************
     * Resource *
     ************/

    LinkImpl::LinkImpl(simgrid::surf::NetworkModel* model, const char* name, lmm_constraint_t constraint)
        : Resource(model, name, constraint), piface_(this)
    {

      if (strcmp(name,"__loopback__"))
        xbt_assert(!LinkImpl::byName(name), "Link '%s' declared several times in the platform.", name);

      latency_.scale   = 1;
      bandwidth_.scale = 1;

      links->insert({name, this});
      XBT_DEBUG("Create link '%s'",name);

    }

    /** @brief use destroy() instead of this destructor */
    LinkImpl::~LinkImpl()
    {
      xbt_assert(currentlyDestroying_, "Don't delete Links directly. Call destroy() instead.");
    }
    /** @brief Fire the required callbacks and destroy the object
     *
     * Don't delete directly a Link, call l->destroy() instead.
     */
    void LinkImpl::destroy()
    {
      if (!currentlyDestroying_) {
        currentlyDestroying_ = true;
        s4u::Link::onDestruction(this->piface_);
        delete this;
      }
    }

    bool LinkImpl::isUsed()
    {
      return lmm_constraint_used(model()->getMaxminSystem(), constraint());
    }

    double LinkImpl::latency()
    {
      return latency_.peak * latency_.scale;
    }

    double LinkImpl::bandwidth()
    {
      return bandwidth_.peak * bandwidth_.scale;
    }

    int LinkImpl::sharingPolicy()
    {
      return lmm_constraint_sharing_policy(constraint());
    }

    void LinkImpl::turnOn()
    {
      if (isOff()) {
        Resource::turnOn();
        s4u::Link::onStateChange(this->piface_);
      }
    }
    void LinkImpl::turnOff()
    {
      if (isOn()) {
        Resource::turnOff();
        s4u::Link::onStateChange(this->piface_);
      }
    }
    void LinkImpl::setStateTrace(tmgr_trace_t trace)
    {
      xbt_assert(stateEvent_ == nullptr, "Cannot set a second state trace to Link %s", cname());
      stateEvent_ = future_evt_set->add_trace(trace, this);
    }
    void LinkImpl::setBandwidthTrace(tmgr_trace_t trace)
    {
      xbt_assert(bandwidth_.event == nullptr, "Cannot set a second bandwidth trace to Link %s", cname());
      bandwidth_.event = future_evt_set->add_trace(trace, this);
    }
    void LinkImpl::setLatencyTrace(tmgr_trace_t trace)
    {
      xbt_assert(latency_.event == nullptr, "Cannot set a second latency trace to Link %s", cname());
      latency_.event = future_evt_set->add_trace(trace, this);
    }


    /**********
     * Action *
     **********/

    void NetworkAction::setState(Action::State state)
    {
      Action::setState(state);
      s4u::Link::onCommunicationStateChange(this);
    }

    /** @brief returns a list of all Links that this action is using */
    std::list<LinkImpl*> NetworkAction::links()
    {
      std::list<LinkImpl*> retlist;
      lmm_system_t sys = getModel()->getMaxminSystem();
      int llen         = lmm_get_number_of_cnst_from_var(sys, getVariable());

      for (int i = 0; i < llen; i++) {
        /* Beware of composite actions: ptasks put links and cpus together */
        // extra pb: we cannot dynamic_cast from void*...
        Resource* resource = static_cast<Resource*>(lmm_constraint_id(lmm_get_cnst_from_var(sys, getVariable(), i)));
        LinkImpl* link     = dynamic_cast<LinkImpl*>(resource);
        if (link != nullptr)
          retlist.push_back(link);
      }

      return retlist;
    }
  }
}

#endif /* NETWORK_INTERFACE_CPP_ */
