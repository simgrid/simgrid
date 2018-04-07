/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_interface.hpp"
#include "simgrid/sg_config.hpp"
#include "src/surf/surf_interface.hpp"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf, "Logging specific to the SURF network module");

namespace simgrid {
namespace kernel {
namespace resource {

/* List of links */
std::unordered_map<std::string, LinkImpl*>* LinkImpl::links = new std::unordered_map<std::string, LinkImpl*>();

LinkImpl* LinkImpl::byName(std::string name)
{
  auto link = links->find(name);
  return link == links->end() ? nullptr : link->second;
  }
  /** @brief Returns the amount of links in the platform */
  int LinkImpl::linksCount()
  {
    return links->size();
  }
  void LinkImpl::linksList(std::vector<s4u::Link*>* linkList)
  {
    for (auto const& kv : *links) {
      linkList->push_back(&kv.second->piface_);
    }
  }

  /** @brief Returns a list of all existing links */
  LinkImpl** LinkImpl::linksList()
  {
    LinkImpl** res = xbt_new(LinkImpl*, (int)links->size());
    int i          = 0;
    for (auto const& kv : *links) {
      res[i] = kv.second;
      i++;
    }
    return res;
  }
  /** @brief destructor of the static data */
  void LinkImpl::linksExit()
  {
    for (auto const& kv : *links)
      (kv.second)->destroy();
    delete links;
  }
  }
}
} // namespace simgrid

/*********
 * Model *
 *********/

simgrid::kernel::resource::NetworkModel* surf_network_model = nullptr;

namespace simgrid {
namespace kernel {
namespace resource {

/** @brief Command-line option 'network/TCP-gamma' -- see \ref options_model_network_gamma */
simgrid::config::Flag<double> NetworkModel::cfg_tcp_gamma(
    {"network/TCP-gamma", "network/TCP_gamma"},
    "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; "
    "Use the last given value, which is the max window size)",
    4194304.0);

/** @brief Command-line option 'network/crosstraffic' -- see \ref options_model_network_crosstraffic */
simgrid::config::Flag<bool> NetworkModel::cfg_crosstraffic(
    "network/crosstraffic",
    "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)", "yes");

NetworkModel::~NetworkModel() = default;

double NetworkModel::latencyFactor(double /*size*/)
{
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
      double minRes = Model::next_occuring_event_full(now);

      for (Action const& action : *get_running_action_set()) {
        const NetworkAction& net_action = static_cast<const NetworkAction&>(action);
        if (net_action.latency_ > 0)
          minRes = (minRes < 0) ? net_action.latency_ : std::min(minRes, net_action.latency_);
      }

      XBT_DEBUG("Min of share resources %f", minRes);

      return minRes;
    }

    /************
     * Resource *
     ************/

    LinkImpl::LinkImpl(NetworkModel* model, const std::string& name, lmm::Constraint* constraint)
        : Resource(model, name, constraint), piface_(this)
    {

      if (name != "__loopback__")
        xbt_assert(not LinkImpl::byName(name), "Link '%s' declared several times in the platform.", name.c_str());

      latency_.scale   = 1;
      bandwidth_.scale = 1;

      links->insert({name, this});
      XBT_DEBUG("Create link '%s'", name.c_str());
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
      if (not currentlyDestroying_) {
        currentlyDestroying_ = true;
        s4u::Link::onDestruction(this->piface_);
        delete this;
      }
    }

    bool LinkImpl::is_used()
    {
      return get_model()->get_maxmin_system()->constraint_used(get_constraint());
    }

    double LinkImpl::latency()
    {
      return latency_.peak * latency_.scale;
    }

    double LinkImpl::bandwidth()
    {
      return bandwidth_.peak * bandwidth_.scale;
    }

    s4u::Link::SharingPolicy LinkImpl::sharingPolicy()
    {
      return get_constraint()->get_sharing_policy();
    }

    void LinkImpl::turn_on()
    {
      if (is_off()) {
        Resource::turn_on();
        s4u::Link::onStateChange(this->piface_);
      }
    }
    void LinkImpl::turn_off()
    {
      if (is_on()) {
        Resource::turn_off();
        s4u::Link::onStateChange(this->piface_);
      }
    }
    void LinkImpl::setStateTrace(tmgr_trace_t trace)
    {
      xbt_assert(stateEvent_ == nullptr, "Cannot set a second state trace to Link %s", get_cname());
      stateEvent_ = future_evt_set->add_trace(trace, this);
    }
    void LinkImpl::setBandwidthTrace(tmgr_trace_t trace)
    {
      xbt_assert(bandwidth_.event == nullptr, "Cannot set a second bandwidth trace to Link %s", get_cname());
      bandwidth_.event = future_evt_set->add_trace(trace, this);
    }
    void LinkImpl::setLatencyTrace(tmgr_trace_t trace)
    {
      xbt_assert(latency_.event == nullptr, "Cannot set a second latency trace to Link %s", get_cname());
      latency_.event = future_evt_set->add_trace(trace, this);
    }


    /**********
     * Action *
     **********/

    void NetworkAction::set_state(Action::State state)
    {
      Action::set_state(state);
      s4u::Link::onCommunicationStateChange(this);
    }

    /** @brief returns a list of all Links that this action is using */
    std::list<LinkImpl*> NetworkAction::links()
    {
      std::list<LinkImpl*> retlist;
      int llen = get_variable()->get_number_of_constraint();

      for (int i = 0; i < llen; i++) {
        /* Beware of composite actions: ptasks put links and cpus together */
        // extra pb: we cannot dynamic_cast from void*...
        Resource* resource = static_cast<Resource*>(get_variable()->get_constraint(i)->get_id());
        LinkImpl* link     = dynamic_cast<LinkImpl*>(resource);
        if (link != nullptr)
          retlist.push_back(link);
      }

      return retlist;
    }
  }
  } // namespace kernel
}

#endif /* NETWORK_INTERFACE_CPP_ */
