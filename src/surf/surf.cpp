#include "surf.hpp"
#include "cpu.hpp"
#include "simix/smx_host_private.h"

XBT_LOG_NEW_CATEGORY(surfpp, "All SURF categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surfpp_kernel, surfpp,
                                "Logging specific to SURF (kernel)");

/*********
 * Utils *
 *********/

double NOWW = 0;

XBT_INLINE double surf_get_clock(void)
{
  return NOWW;
}

/* This function is a pimple that we ought to fix. But it won't be easy.
 *
 * The surf_solve() function does properly return the set of actions that changed.
 * Instead, each model change a global data, and then the caller of surf_solve must
 * pick into these sets of action_failed and action_done.
 *
 * This was not clean but ok as long as we didn't had to restart the processes when the resource comes back up.
 * We worked by putting sentinel actions on every resources we are interested in,
 * so that surf informs us if/when the corresponding resource fails.
 *
 * But this does not work to get Simix informed of when a resource comes back up, and this is where this pimple comes.
 * We have a set of resources that are currently down and for which simix needs to know when it comes back up.
 * And the current function is called *at every simulation step* to sweep over that set, searching for a resource
 * that was turned back up in the meanwhile. This is UGLY and slow.
 *
 * The proper solution would be to not rely on globals for the action_failed and action_done swags.
 * They must be passed as parameter by the caller (the handling of these actions in simix may let you
 * think that these two sets can be merged, but their handling in SimDag induce the contrary unless this
 * simdag code can check by itself whether the action is done of failed -- seems very doable, but yet more
 * cleanup to do).
 *
 * Once surf_solve() is passed the set of actions that changed, you want to add a new set of resources back up
 * as parameter to this function. You also want to add a boolean field "restart_watched" to each resource, and
 * make sure that whenever a resource with this field enabled comes back up, it's added to that set so that Simix
 * sees it and react accordingly. This would kill that need for surf to call simix.
 *
 */

static void remove_watched_host(void *key)
{
  xbt_dict_remove(watched_hosts_lib, *(char**)key);
}

/*TODO: keepit void surf_watched_hosts(void)
{
  char *key;
  void *_host;
  smx_host_t host;
  xbt_dict_cursor_t cursor;
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(char*), NULL);

  XBT_DEBUG("Check for host SURF_RESOURCE_ON on watched_hosts_lib");
  xbt_dict_foreach(watched_hosts_lib,cursor,key,_host)
  {
    host = (smx_host_t) host;
    if(SIMIX_host_get_state(host) == SURF_RESOURCE_ON){
      XBT_INFO("Restart processes on host: %s",SIMIX_host_get_name(host));
      SIMIX_host_autorestart(host);
      xbt_dynar_push_as(hosts, char*, key);
    }
    else
      XBT_DEBUG("See SURF_RESOURCE_OFF on host: %s",key);
  }
  xbt_dynar_map(hosts, remove_watched_host);
  xbt_dynar_free(&hosts);
}*/

/*********
 * Model *
 *********/

double Model::shareResourcesLazy(double now)
{
  ActionLmmPtr action = NULL;
  double min = -1;
  double value;

  XBT_DEBUG
      ("Before share resources, the size of modified actions set is %d",
       xbt_swag_size(p_modifiedSet));

  lmm_solve(p_maxminSystem);

  XBT_DEBUG
      ("After share resources, The size of modified actions set is %d",
       xbt_swag_size(p_modifiedSet));

  while((action = (ActionLmmPtr) xbt_swag_extract(p_modifiedSet))) {
    int max_dur_flag = 0;

    if (action->p_stateSet != p_runningActionSet)
      continue;

    /* bogus priority, skip it */
    if (action->m_priority <= 0)
      continue;

    action->updateRemainingLazy(now);

    min = -1;
    value = lmm_variable_getvalue(action->p_variable);
    if (value > 0) {
      if (action->m_remains > 0) {
        value = action->m_remains / value;
        min = now + value;
      } else {
        value = 0.0;
        min = now;
      }
    }

    if ((action->m_maxDuration != NO_MAX_DURATION)
        && (min == -1
            || action->m_start +
            action->m_maxDuration < min)) {
      min = action->m_start +
          action->m_maxDuration;
      max_dur_flag = 1;
    }

    XBT_DEBUG("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
        action->m_start, now + value,
        action->m_maxDuration);

    if (min != -1) {
      action->heapRemove(p_actionHeap);
      action->heapInsert(p_actionHeap, min, max_dur_flag ? MAX_DURATION : NORMAL);
      XBT_DEBUG("Insert at heap action(%p) min %lf now %lf", action, min,
                now);
    } else DIE_IMPOSSIBLE;
  }

  //hereafter must have already the min value for this resource model
  if (xbt_heap_size(p_actionHeap) > 0)
    min = xbt_heap_maxkey(p_actionHeap) - now;
  else
    min = -1;

  XBT_DEBUG("The minimum with the HEAP %lf", min);

  return min;
}

double Model::shareResourcesFull(xbt_swag_t running_actions,
                          size_t offset,
                          lmm_system_t sys,
                          void (*solve) (lmm_system_t))
{
  void *_action = NULL;
  ActionPtr action = NULL;
  double min = -1;
  double value = -1;
#define VARIABLE(action) (*((lmm_variable_t*)(((char *) (action)) + (offset))))

  solve(sys);

  xbt_swag_foreach(_action, running_actions) {
    action = (ActionPtr)_action;
    value = lmm_variable_getvalue(VARIABLE(action));
    if ((value > 0) || (action->m_maxDuration >= 0))
      break;
  }

  if (!action)
    return -1.0;

  if (value > 0) {
    if (action->m_remains > 0)
      min = action->m_remains / value;
    else
      min = 0.0;
    if ((action->m_maxDuration >= 0) && (action->m_maxDuration < min))
      min = action->m_maxDuration;
  } else
    min = action->m_maxDuration;


  for (action = (ActionPtr) xbt_swag_getNext(action, running_actions->offset);
       action;
       action = (ActionPtr) xbt_swag_getNext(action, running_actions->offset)) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if (value > 0) {
      if (action->m_remains > 0)
        value = action->m_remains / value;
      else
        value = 0.0;
      if (value < min) {
        min = value;
        XBT_DEBUG("Updating min (value) with %p: %f", action, min);
      }
    }
    if ((action->m_maxDuration >= 0) && (action->m_maxDuration < min)) {
      min = action->m_maxDuration;
      XBT_DEBUG("Updating min (duration) with %p: %f", action, min);
    }
  }
  XBT_DEBUG("min value : %f", min);

#undef VARIABLE
  return min;
}

void Model::gapRemove(ActionLmmPtr action) {}


void Model::updateActionsStateLazy(double now, double delta)
{

}

void Model::updateActionsStateFull(double now, double delta)
{

}


void Model::addTurnedOnCallback(ResourceCallback rc)
{
  m_resOnCB = rc;
}

void Model::notifyResourceTurnedOn(ResourcePtr r)
{
  m_resOnCB(r);
}

void Model::addTurnedOffCallback(ResourceCallback rc)
{
  m_resOffCB = rc;
}

void Model::notifyResourceTurnedOff(ResourcePtr r)
{
  m_resOffCB(r);
}

void Model::addActionCancelCallback(ActionCallback ac)
{
  m_actCancelCB = ac;
}

void Model::notifyActionCancel(ActionPtr a)
{
  m_actCancelCB(a);
}

void Model::addActionResumeCallback(ActionCallback ac)
{
  m_actResumeCB = ac;
}

void Model::notifyActionResume(ActionPtr a)
{
  m_actResumeCB(a);
}

void Model::addActionSuspendCallback(ActionCallback ac)
{
  m_actSuspendCB = ac;
}

void Model::notifyActionSuspend(ActionPtr a)
{
  m_actSuspendCB(a);
}


/************
 * Resource *
 ************/

string Resource::getName() {
  return m_name;
}

e_surf_resource_state_t Resource::getState()
{
  return m_stateCurrent;
}

bool Resource::isOn()
{
  return m_running;
}

void Resource::turnOn()
{
  if (!m_running) {
    m_running = true;
    p_model->notifyResourceTurnedOn(this);
  }
}

void Resource::turnOff()
{
  if (m_running) {
    m_running = false;
    p_model->notifyResourceTurnedOff(this);
  }
}

/**********
 * Action *
 **********/
/* added to manage the communication action's heap */
void surf_action_lmm_update_index_heap(void *action, int i) {
  ((ActionLmmPtr)action)->updateIndexHeap(i);
}

void ActionLmm::updateIndexHeap(int i)
{
  m_indexHeap = i;
}

/*TODO/const char *surf_action_state_names[6] = {
  "SURF_ACTION_READY",
  "SURF_ACTION_RUNNING",
  "SURF_ACTION_FAILED",
  "SURF_ACTION_DONE",
  "SURF_ACTION_TO_FREE",
  "SURF_ACTION_NOT_IN_THE_SYSTEM"
};*/

e_surf_action_state_t Action::getState()
{
  if (p_stateSet ==  p_model->p_readyActionSet)
    return SURF_ACTION_READY;
  if (p_stateSet ==  p_model->p_runningActionSet)
    return SURF_ACTION_RUNNING;
  if (p_stateSet ==  p_model->p_failedActionSet)
    return SURF_ACTION_FAILED;
  if (p_stateSet ==  p_model->p_doneActionSet)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

void Action::setState(e_surf_action_state_t state)
{
  //surf_action_state_t action_state = &(action->model_type->states);
  XBT_IN("(%p,%s)", this, surf_action_state_names[state]);
  xbt_swag_remove(this, p_stateSet);

  if (state == SURF_ACTION_READY)
    p_stateSet = p_model->p_readyActionSet;
  else if (state == SURF_ACTION_RUNNING)
    p_stateSet = p_model->p_runningActionSet;
  else if (state == SURF_ACTION_FAILED)
    p_stateSet = p_model->p_failedActionSet;
  else if (state == SURF_ACTION_DONE)
    p_stateSet = p_model->p_doneActionSet;
  else
    p_stateSet = NULL;

  if (p_stateSet)
    xbt_swag_insert(this, p_stateSet);
  XBT_OUT();
}

double Action::getStartTime()
{
  return m_start;
}

double Action::getFinishTime()
{
  /* keep the function behavior, some models (cpu_ti) change the finish time before the action end */
  return m_remains == 0 ? m_finish : -1;
}

void Action::setData(void* data)
{
  p_data = data;
}

#ifdef HAVE_TRACING
void Action::setCategory(const char *category)
{
  XBT_IN("(%p,%s)", this, category);
  p_category = xbt_strdup(category);
  XBT_OUT();
}
#endif

/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
void ActionLmm::heapInsert(xbt_heap_t heap, double key, enum heap_action_type hat)
{
  m_hat = hat;
  xbt_heap_push(heap, this, key);
}

void ActionLmm::heapRemove(xbt_heap_t heap)
{
  m_hat = NOTSET;
  if (m_indexHeap >= 0) {
    xbt_heap_remove(heap, m_indexHeap);
  }
}

double ActionLmm::getRemains()
{
  XBT_IN("(%p)", this);
  /* update remains before return it */
  if (p_updateMechanism == UM_LAZY)      /* update remains before return it */
    updateRemainingLazy(surf_get_clock());
  XBT_OUT();
  return m_remains;
}

/*void Action::cancel()
{
  p_model->notifyActionCancel(this);
}

void Action::suspend()
{
  p_model->notifyActionSuspend(this);
}

void Action::resume()
{
  p_model->notifyActionResume(this);
}

bool Action::isSuspended()
{
  return false;
}*/

