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
    }

    double NetworkConstantModel::next_occuring_event(double /*now*/)
    {
      NetworkConstantAction *action = NULL;
      double min = -1.0;

      ActionList *actionSet = getRunningActionSet();
      for(ActionList::iterator it(actionSet->begin()), itend(actionSet->end())
          ; it != itend ; ++it) {
        action = static_cast<NetworkConstantAction*>(&*it);
        if (action->m_latency > 0 && (min < 0 || action->m_latency < min))
          min = action->m_latency;
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
        if (action->m_latency > 0) {
          if (action->m_latency > delta) {
            double_update(&(action->m_latency), delta, sg_surf_precision);
          } else {
            action->m_latency = 0.0;
          }
        }
        action->updateRemains(action->getCost() * delta / action->m_latInit);
        if (action->getMaxDuration() != NO_MAX_DURATION)
          action->updateMaxDuration(delta);

        if (action->getRemainsNoUpdate() <= 0) {
          action->finish();
          action->setState(SURF_ACTION_DONE);
        } else if ((action->getMaxDuration() != NO_MAX_DURATION)
            && (action->getMaxDuration() <= 0)) {
          action->finish();
          action->setState(SURF_ACTION_DONE);
        }
      }
    }

    Action *NetworkConstantModel::communicate(NetCard *src, NetCard *dst,
        double size, double rate)
    {
      char *src_name = src->name();
      char *dst_name = dst->name();

      XBT_IN("(%s,%s,%g,%g)", src_name, dst_name, size, rate);
      NetworkConstantAction *action = new NetworkConstantAction(this, size, sg_latency_factor);
      XBT_OUT();

      networkCommunicateCallbacks(action, src, dst, size, rate);
      return action;
    }

    /**********
     * Action *
     **********/

    int NetworkConstantAction::unref()
    {
      m_refcount--;
      if (!m_refcount) {
        if (action_hook.is_linked())
          p_stateSet->erase(p_stateSet->iterator_to(*this));
        delete this;
        return 1;
      }
      return 0;
    }

    void NetworkConstantAction::cancel()
    {
      return;
    }

  }
}
