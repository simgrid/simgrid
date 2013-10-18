#include "cpu.hpp"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF cpu module");
}

CpuModelPtr surf_cpu_model;

/*********
 * Model *
 *********/

void CpuModel::updateActionsStateLazy(double now, double delta)
{
  void *_action;
  ActionLmmPtr action;
  while ((xbt_heap_size(p_actionHeap) > 0)
         && (double_equals(xbt_heap_maxkey(p_actionHeap), now))) {
    action = (ActionLmmPtr) xbt_heap_pop(p_actionHeap);
    XBT_DEBUG("Something happened to action %p", action);
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      CpuPtr cpu = (CpuPtr) lmm_constraint_id(lmm_get_cnst_from_var(p_maxminSystem, action->p_variable, 0));
      TRACE_surf_host_set_utilization(cpu->m_name, action->p_category,
                                      lmm_variable_getvalue(action->p_variable),
                                      action->m_lastUpdate,
                                      now - action->m_lastUpdate);
    }
#endif

    action->m_finish = surf_get_clock();
    XBT_DEBUG("Action %p finished", action);

    /* set the remains to 0 due to precision problems when updating the remaining amount */
    action->m_remains = 0;
    action->setState(SURF_ACTION_DONE);
    action->heapRemove(p_actionHeap); //FIXME: strange call since action was already popped
  }
#ifdef HAVE_TRACING
  if (TRACE_is_enabled()) {
    //defining the last timestamp that we can safely dump to trace file
    //without losing the event ascending order (considering all CPU's)
    double smaller = -1;
    xbt_swag_foreach(_action, p_runningActionSet) {
      action = (ActionLmmPtr) _action;
        if (smaller < 0) {
          smaller = action->m_lastUpdate;
          continue;
        }
        if (action->m_lastUpdate < smaller) {
          smaller = action->m_lastUpdate;
        }
    }
    if (smaller > 0) {
      TRACE_last_timestamp_to_dump = smaller;
    }
  }
#endif
  return;
}

void CpuModel::updateActionsStateFull(double now, double delta)
{
  void *_action, *_next_action;
  ActionLmmPtr action = NULL;
  ActionLmmPtr next_action = NULL;
  xbt_swag_t running_actions = p_runningActionSet;

  xbt_swag_foreach_safe(_action, _next_action, running_actions) {
    action = dynamic_cast<ActionLmmPtr>(static_cast<ActionPtr>(_action));
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      CpuPtr x = (CpuPtr) lmm_constraint_id(lmm_get_cnst_from_var
                              (p_maxminSystem, action->p_variable, 0));

      TRACE_surf_host_set_utilization(x->m_name,
                                      action->p_category,
                                      lmm_variable_getvalue(action->p_variable),
                                      now - delta,
                                      delta);
      TRACE_last_timestamp_to_dump = now - delta;
    }
#endif

    double_update(&(action->m_remains),
                  lmm_variable_getvalue(action->p_variable) * delta);


    if (action->m_maxDuration != NO_MAX_DURATION)
      double_update(&(action->m_maxDuration), delta);


    if ((action->m_remains <= 0) &&
        (lmm_get_variable_weight(action->p_variable) > 0)) {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);

    } else if ((action->m_maxDuration != NO_MAX_DURATION) &&
               (action->m_maxDuration <= 0)) {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
    }
  }

  return;
}

/************
 * Resource *
 ************/

double Cpu::getSpeed(double load)
{
  return load * m_powerPeak;
}

double Cpu::getAvailableSpeed()
{
/* number between 0 and 1 */
  return m_powerScale;
}

int Cpu::getCore()
{
  return m_core;
}

/**********
 * Action *
 **********/

void CpuActionLmm::updateRemainingLazy(double now)
{
  double delta = 0.0;

  xbt_assert(p_stateSet == p_model->p_runningActionSet,
      "You're updating an action that is not running.");

  /* bogus priority, skip it */
  xbt_assert(m_priority > 0,
      "You're updating an action that seems suspended.");

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_DEBUG("Updating action(%p): remains was %lf, last_update was: %lf", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta);

#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      CpuPtr cpu = (CpuPtr) lmm_constraint_id(lmm_get_cnst_from_var(p_model->p_maxminSystem, p_variable, 0));
      TRACE_surf_host_set_utilization(cpu->m_name, p_category, m_lastValue, m_lastUpdate, now - m_lastUpdate);
    }
#endif
    XBT_DEBUG("Updating action(%p): remains is now %lf", this, m_remains);
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(p_variable);
}
