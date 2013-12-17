#include "network_constant.hpp"
#include "surf/random_mgr.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static int host_number_int = 0;

static void netcste_count_hosts(sg_platf_host_cbarg_t /*h*/) {
  host_number_int++;
}

/*********
 * Model *
 *********/
void surf_network_model_init_Constant()
{
  xbt_assert(surf_network_model == NULL);
  surf_network_model = new NetworkConstantModel();

  sg_platf_host_add_cb(netcste_count_hosts);

  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);
}

double NetworkConstantModel::shareResources(double /*now*/)
{
  NetworkConstantActionLmmPtr action = NULL;
  double min = -1.0;

  ActionListPtr actionSet = getRunningActionSet();
  for(ActionList::iterator it(actionSet->begin()), itend(actionSet->end())
	 ; it != itend ; ++it) {
	action = dynamic_cast<NetworkConstantActionLmmPtr>(&*it);
    if (action->m_latency > 0) {
      if (min < 0)
        min = action->m_latency;
      else if (action->m_latency < min)
        min = action->m_latency;
    }
  }

  return min;
}

void NetworkConstantModel::updateActionsState(double /*now*/, double delta)
{
  NetworkConstantActionLmmPtr action = NULL;
  ActionListPtr actionSet = getRunningActionSet();
  for(ActionList::iterator it(actionSet->begin()), itNext=it, itend(actionSet->end())
     ; it != itend ; it=itNext) {
    ++itNext;
	action = dynamic_cast<NetworkConstantActionLmmPtr>(&*it);
    if (action->m_latency > 0) {
      if (action->m_latency > delta) {
        double_update(&(action->m_latency), delta);
      } else {
        action->m_latency = 0.0;
      }
    }
    action->updateRemains(action->getCost() * delta / action->m_latInit);
    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    if (action->getRemains() <= 0) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->getMaxDuration() != NO_MAX_DURATION)
               && (action->getMaxDuration() <= 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    }
  }
}

ActionPtr NetworkConstantModel::communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                         double size, double rate)
{
  char *src_name = src->p_name;
  char *dst_name = dst->p_name;

  XBT_IN("(%s,%s,%g,%g)", src_name, dst_name, size, rate);
  NetworkConstantActionLmmPtr action = new NetworkConstantActionLmm(this, size, sg_latency_factor);
  XBT_OUT();

  return action;
}

/************
 * Resource *
 ************/
bool NetworkConstantLinkLmm::isUsed()
{
  return 0;
}

void NetworkConstantLinkLmm::updateState(tmgr_trace_event_t /*event_type*/,
                                         double /*value*/, double /*time*/)
{
  DIE_IMPOSSIBLE;
}

double NetworkConstantLinkLmm::getBandwidth()
{
  DIE_IMPOSSIBLE;
  return -1.0; /* useless since DIE actually abort(), but eclipse prefer to have a useless and harmless return */
}

double NetworkConstantLinkLmm::getLatency()
{
  DIE_IMPOSSIBLE;
  return -1.0; /* useless since DIE actually abort(), but eclipse prefer to have a useless and harmless return */
}

bool NetworkConstantLinkLmm::isShared()
{
  DIE_IMPOSSIBLE;
  return -1; /* useless since DIE actually abort(), but eclipse prefer to have a useless and harmless return */
}

/**********
 * Action *
 **********/

int NetworkConstantActionLmm::unref()
{
  m_refcount--;
  if (!m_refcount) {
	if (actionHook::is_linked())
	  p_stateSet->erase(p_stateSet->iterator_to(*this));
    delete this;
  return 1;
  }
  return 0;
}

void NetworkConstantActionLmm::cancel()
{
  return;
}

#ifdef HAVE_TRACING
void NetworkConstantActionLmm::setCategory(const char */*category*/)
{
  //ignore completely the categories in constant model, they are not traced
}
#endif

void NetworkConstantActionLmm::suspend()
{
  m_suspended = true;
}

void NetworkConstantActionLmm::resume()
{
  if (m_suspended)
	m_suspended = false;
}

void NetworkConstantActionLmm::recycle()
{
  return;
}

bool NetworkConstantActionLmm::isSuspended()
{
  return m_suspended;
}

