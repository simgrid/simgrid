#include "cpu.hpp"

extern "C" {
XBT_LOG_EXTERNAL_CATEGORY(surf_kernel);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF cpu module");
}

CpuModelPtr surf_cpu_model_pm;
CpuModelPtr surf_cpu_model_vm;

/*********
 * Model *
 *********/

void CpuModel::updateActionsStateLazy(double now, double delta)
{
  void *_action;
  CpuActionLmmPtr action;
  while ((xbt_heap_size(p_actionHeap) > 0)
         && (double_equals(xbt_heap_maxkey(p_actionHeap), now))) {
    action = dynamic_cast<CpuActionLmmPtr>(static_cast<ActionLmmPtr>(xbt_heap_pop(p_actionHeap)));
    XBT_CDEBUG(surf_kernel, "Something happened to action %p", action);
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
    XBT_CDEBUG(surf_kernel, "Action %p finished", action);

    action->updateEnergy();

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
      action = dynamic_cast<CpuActionLmmPtr>(static_cast<ActionPtr>(_action));
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
  CpuActionLmmPtr action = NULL;
  xbt_swag_t running_actions = p_runningActionSet;

  xbt_swag_foreach_safe(_action, _next_action, running_actions) {
    action = dynamic_cast<CpuActionLmmPtr>(static_cast<ActionPtr>(_action));
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
    action->updateEnergy();
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
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains was %lf, last_update was: %lf", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta);

#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      CpuPtr cpu = (CpuPtr) lmm_constraint_id(lmm_get_cnst_from_var(p_model->p_maxminSystem, p_variable, 0));
      TRACE_surf_host_set_utilization(cpu->m_name, p_category, m_lastValue, m_lastUpdate, now - m_lastUpdate);
    }
#endif
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains is now %lf", this, m_remains);
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(p_variable);
}

void CpuActionLmm::setBound(double bound)
{
  XBT_IN("(%p,%g)", this, bound);
  m_bound = bound;
  lmm_update_variable_bound(p_model->p_maxminSystem, p_variable, bound);

  if (p_model->p_updateMechanism == UM_LAZY)
	heapRemove(p_model->p_actionHeap);
  XBT_OUT();
}

/*
 *
 * This function formulates a constraint problem that pins a given task to
 * particular cores. Currently, it is possible to pin a task to an exactly one
 * specific core. The system links the variable object of the task to the
 * per-core constraint object.
 *
 * But, the taskset command on Linux takes a mask value specifying a CPU
 * affinity setting of a given task. If the mask value is 0x03, the given task
 * will be executed on the first core (CPU0) or the second core (CPU1) on the
 * given PM. The schedular will determine appropriate placements of tasks,
 * considering given CPU affinities and task activities.
 *
 * How should the system formulate constraint problems for an affinity to
 * multiple cores?
 *
 * The cpu argument must be the host where the task is being executed. The
 * action object does not have the information about the location where the
 * action is being executed.
 */
void CpuActionLmm::setAffinity(CpuLmmPtr cpu, unsigned long mask)
{
  lmm_variable_t var_obj = p_variable;

  XBT_IN("(%p,%lx)", this, mask);

  {
    unsigned long nbits = 0;

    /* FIXME: There is much faster algorithms doing this. */
    unsigned long i;
    for (i = 0; i < cpu->m_core; i++) {
      unsigned long has_affinity = (1UL << i) & mask;
      if (has_affinity)
        nbits += 1;
    }

    if (nbits > 1) {
      XBT_CRITICAL("Do not specify multiple cores for an affinity mask.");
      XBT_CRITICAL("See the comment in cpu_action_set_affinity().");
      DIE_IMPOSSIBLE;
    }
  }



  unsigned long i;
  for (i = 0; i < cpu->m_core; i++) {
    XBT_DEBUG("clear affinity %p to cpu-%lu@%s", this, i,  cpu->m_name);
    lmm_shrink(cpu->p_model->p_maxminSystem, cpu->p_constraintCore[i], var_obj);

    unsigned long has_affinity = (1UL << i) & mask;
    if (has_affinity) {
      /* This function only accepts an affinity setting on the host where the
       * task is now running. In future, a task might move to another host.
       * But, at this moment, this function cannot take an affinity setting on
       * that future host.
       *
       * It might be possible to extend the code to allow this function to
       * accept affinity settings on a future host. We might be able to assign
       * zero to elem->value to maintain such inactive affinity settings in the
       * system. But, this will make the system complex. */
      XBT_DEBUG("set affinity %p to cpu-%lu@%s", this, i, cpu->m_name);
      lmm_expand(cpu->p_model->p_maxminSystem, cpu->p_constraintCore[i], var_obj, 1.0);
    }
  }

  if (cpu->p_model->p_updateMechanism == UM_LAZY) {
    /* FIXME (hypervisor): Do we need to do something for the LAZY mode? */
  }

  XBT_OUT();
}
