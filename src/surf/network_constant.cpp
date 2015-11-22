/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_constant.hpp"
#include "surf/random_mgr.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static int host_number_int = 0;

static void netcste_count_hosts(sg_platf_host_cbarg_t /*h*/) {
  host_number_int++;
}
static void netcste_parse_nolink(sg_platf_link_cbarg_t link){
	xbt_die("There is no link in the Constant network model. "
			"Please remove any link from your platform (and switch to routing='None')");
}

/*********
 * Model *
 *********/
void surf_network_model_init_Constant()
{
  xbt_assert(surf_network_model == NULL);
  surf_network_model = new NetworkConstantModel();

  routing_model_create(NULL);

  sg_platf_host_add_cb(netcste_count_hosts);
  sg_platf_link_add_cb(netcste_parse_nolink);

  Model *model = surf_network_model;
  xbt_dynar_push(all_existing_models, &model);
}

double NetworkConstantModel::shareResources(double /*now*/)
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

Action *NetworkConstantModel::communicate(RoutingEdge *src, RoutingEdge *dst,
		                         double size, double rate)
{
  char *src_name = src->getName();
  char *dst_name = dst->getName();

  XBT_IN("(%s,%s,%g,%g)", src_name, dst_name, size, rate);
  NetworkConstantAction *action = new NetworkConstantAction(this, size, sg_latency_factor);
  XBT_OUT();

  surf_callback_emit(networkCommunicateCallbacks, action, src, dst, size, rate);
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

void NetworkConstantAction::setCategory(const char * /*category*/)
{
  //ignore completely the categories in constant model, they are not traced
}

void NetworkConstantAction::suspend()
{
  m_suspended = true;
}

void NetworkConstantAction::resume()
{
  if (m_suspended)
	m_suspended = false;
}

bool NetworkConstantAction::isSuspended()
{
  return m_suspended;
}

