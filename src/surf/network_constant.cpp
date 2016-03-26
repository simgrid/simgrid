/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_constant.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

/*********
 * Model *
 *********/
void surf_network_model_init_Constant()
{
  xbt_assert(surf_network_model == NULL);
  surf_network_model = new simgrid::surf::NetworkConstantModel();
  xbt_dynar_push(all_existing_models, &surf_network_model);

  routing_model_create(NULL);

  simgrid::surf::on_link.connect(netlink_parse_init);
}

namespace simgrid {
  namespace surf {

    Link* NetworkConstantModel::createLink(const char *name, double bw, double lat, e_surf_link_sharing_policy_t policy,
        xbt_dict_t properties) {

      xbt_die("Refusing to create the link %s: there is no link in the Constant network model. "
          "Please remove any link from your platform (and switch to routing='None')", name);
      return nullptr;
    }

    double NetworkConstantModel::next_occuring_event(double /*now*/)
    {
      NetworkConstantAction *action = NULL;
      double min = -1.0;

      ActionList *actionSet = getRunningActionSet();
      for(ActionList::iterator it(actionSet->begin()), itend(actionSet->end())
          ; it != itend ; ++it) {
        action = static_cast<NetworkConstantAction*>(&*it);
        if (action->latency_ > 0 && (min < 0 || action->latency_ < min))
          min = action->latency_;
      }

      return min;
    }

    void NetworkConstantModel::updateActionsState(double /*now*/, double delta)
    {
      NetworkConstantAction *action = NULL;
      ActionList *actionSet = getRunningActionSet();
      for(ActionList::iterator it(actionSet->begin()), itNext=it, itend(actionSet->end())
          ; it != itend ; it=itNext) {
        ++itNext;
        action = static_cast<NetworkConstantAction*>(&*it);
        if (action->latency_ > 0) {
          if (action->latency_ > delta) {
            double_update(&(action->latency_), delta, sg_surf_precision);
          } else {
            action->latency_ = 0.0;
          }
        }
        action->updateRemains(action->getCost() * delta / action->initialLatency_);
        if (action->getMaxDuration() != NO_MAX_DURATION)
          action->updateMaxDuration(delta);

        if (action->getRemainsNoUpdate() <= 0) {
          action->finish();
          action->setState(Action::State::done);
        } else if ((action->getMaxDuration() != NO_MAX_DURATION)
            && (action->getMaxDuration() <= 0)) {
          action->finish();
          action->setState(Action::State::done);
        }
      }
    }

    Action *NetworkConstantModel::communicate(NetCard *src, NetCard *dst, double size, double rate)
    {
      NetworkConstantAction *action = new NetworkConstantAction(this, size, sg_latency_factor);

      Link::onCommunicate(action, src, dst);
      return action;
    }

    /**********
     * Action *
     **********/
    NetworkConstantAction::NetworkConstantAction(NetworkConstantModel *model_, double size, double latency)
    : NetworkAction(model_, size, false)
    , initialLatency_(latency)
    {
      latency_ = latency;
      if (latency_ <= 0.0) {
        stateSet_ = getModel()->getDoneActionSet();
        stateSet_->push_back(*this);
      }
    };
  }
}
